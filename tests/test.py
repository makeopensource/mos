from threading import Thread, Lock
import os
import subprocess
import time
import socket
import errno
import shutil
import sys

from termcolor import cprint
from colorama import just_fix_windows_console

just_fix_windows_console()

BASE_PORT = 1111
MAX_PORT = 1234

MAX_INSTANCES = 20

TEST_BINARY_PATH = "./src/"
EXPECTED_PATH = "./expected/"

LOCALHOST = "127.0.0.1"
DATA_DELAY = 0.1
SLICE_SIZE = 128

QEMU_ARGS = ["qemu-system-i386", "-boot", "c", "-no-reboot", "-no-shutdown", "-nographic"]

QEMU_SERIAL_DEV = "tcp:localhost:{port},server"
QEMU_DRIVE = "format=raw,file=./../mOS.bin{copy_num}"

TEST_TIMEOUT = 5

active_instances = 0
instance_mutex = Lock()

# although the ports are assigned iteratively, there are instances where things are in use
# one possibility is that another process is using that port. It will be marked used forever.
# another is that an instance is using it and we've wrapped around. Once that instance ends it will be unmarked. 
used_ports = set()
current_port = BASE_PORT
port_mutex = Lock()

# returns list of tuples where the tuple is (path, filename)
def get_all_files(dir):
    out = []
    
    files = os.scandir(dir)

    for file in files:
        if (file.is_dir()):
            out += get_all_files(file.path)
        elif (file.is_file()):
            if (file.name.endswith(".bin") or file.name.endswith(".expect")):
                out.append((file, file.name))

    files.close()

    return out

def get_expected(expecteds, equivalent):
    for expected in expecteds:
        if (expected[1] == equivalent):
            return expected

    return ("", "")

def get_port():
    global current_port

    with port_mutex:
        if (len(used_ports) > MAX_PORT - BASE_PORT):
            # this should not ever happen.
            print("All ports in use!")
            exit(-1)

        while (current_port in used_ports):
            current_port += 1
            if (current_port > MAX_PORT):
                current_port = BASE_PORT

        port_to_use = current_port
        used_ports.add(port_to_use)  

        current_port += 1
        if (current_port > MAX_PORT):
                current_port = BASE_PORT

        return port_to_use

class TestInstance:
    def __init__(self, port: int, bin_path, expected_path):
        self.port = port
        self.bin_path = bin_path
        self.expected_path = expected_path
        self._result = False
        self._resultLock = Lock()
        self._qemuLock = Lock()
        self._qemu = None
        self.test = None
        self._ready = False
        self._qemu_file = ""

    def beginQemu(self):
        

        command = QEMU_ARGS.copy()
        command.append("-drive")
        command.append(QEMU_DRIVE.format(copy_num=self.port))
        command.append("-serial")
        command.append(QEMU_SERIAL_DEV.format(port=self.port))

        with self._qemuLock:
            self._qemu_file = "./../mOS.bin{copy_num}".format(copy_num=self.port)
            shutil.copyfile("./../mOS.bin", self._qemu_file)
            self._qemu = subprocess.Popen(command, stdin=subprocess.PIPE, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            time.sleep(1) #give qemu some time to open
            self._ready = True
        cprint(self.bin_path.name + " has started.", "green")

    def beginTest(self):
        self.test = Thread(target=test, args=[self])
        self.test.start()

    def start(self):
        global active_instances

        self.beginQemu()
        self.beginTest()

        with instance_mutex:
            active_instances += 1

    def endQemu(self):
        with self._qemuLock:
            if (self._qemu != None and self._qemu.poll() == None):
                try:
                    # send the exit command to QEMU
                    self._qemu.terminate()
                    self._qemu.wait(10)

                except:
                    # force qemu to quit since it refuses to exit normally
                    cprint(self.bin_path.name + " was forcefully closed.", "red", attrs=["bold"])
                    self._qemu.kill()
                    self._qemu.wait(5)

                self._qemu = None
                self._ready = False
                cprint(self.bin_path.name + " has exited.", "cyan")
                os.remove(self._qemu_file)

    def end(self):
        global used_ports, active_instances

        self.endQemu()

        if (self.test != None):
            if (self.test.is_alive()):
                self.test.join()

            self.test = None

        with port_mutex:
            if (self.port in used_ports):
                used_ports.remove(self.port)

    @property
    def result(self):
        with self._resultLock:
            return self._result

    @result.setter
    def result(self, value: bool):
        with self._resultLock:
            self._result = value

    @property
    def ready(self):
        with self._qemuLock:
            return self._ready

    @ready.setter
    def ready(self, value: bool):
        with self._qemuLock:
            self._ready = value

# normal test exit
def test_end_stub(instance: TestInstance, result: bool):
    global active_instances, instance_mutex
    global used_ports, port_mutex

    instance.result = result

    with instance_mutex:
        active_instances -= 1

    instance.endQemu()

    with port_mutex:
        if (instance.port in used_ports):
                used_ports.remove(instance.port)


def test(instance: TestInstance):
    global active_instances, instance_mutex
    global used_ports, current_port, port_mutex

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:

            while (not instance.ready):
                time.sleep(1)

            s.settimeout(TEST_TIMEOUT)
            s.connect((LOCALHOST, instance.port))
            
            time.sleep(1)

            # we have connected to the OS, begin sending data
            # we breakup the binary to send it over multiple packets
            s.send(b"test\0")

            data = s.recv(1)

            while (len(data) < 10 or data[-10:].decode("utf-8") != "begin test"):
                data += s.recv(1)

            with open(instance.bin_path, "rb") as binary:
                bin = binary.read()

                s.send(len(bin).to_bytes(4, "little"))

                chunks = int(len(bin) / SLICE_SIZE)

                for i in range(0, chunks):
                    chunk = bin[i*SLICE_SIZE:(i+1)*SLICE_SIZE]
                    s.send(chunk)
                    time.sleep(DATA_DELAY)

                last = bin[chunks * SLICE_SIZE:]
                if (len(last) > 0):
                    s.send(last)

            with open(instance.expected_path, "r") as expected:
                expect = expected.read()


            if (len(expect) == 0):
                return test_end_stub(instance, True)

            with open(instance.expected_path.path + ".got", "w") as gotF:
                for chr in expect:
                    got = s.recv(1).decode("utf-8")
                    gotF.write(got)
                    if (chr != got):
                        # get as TEST_TIMEOUT worth of data before ending
                        try:
                            start = time.time()
                            while(time.time() - start < TEST_TIMEOUT):
                                gotF.write(s.recv(1).decode("utf-8"))
                        except:
                            pass

                        return test_end_stub(instance, False)

                passed = True
                try:
                    start = time.time()
                    s.settimeout(1);
                    while(time.time() - start < 1.0):
                        gotF.write(s.recv(1).decode("utf-8"))
                        passed = False
                except:
                    pass

            return test_end_stub(instance, passed)

        except socket.timeout:
            cprint(instance.bin_path.name + " | test timed out", "red")

        except socket.error as e:

            if (e.errno == errno.EADDRINUSE or e.errno == errno.ECONNREFUSED):
                # port already in use, get a new one
                instance.endQemu()
                instance.port = get_port()

                instance.beginQemu()
                instance.beginTest()
                return
            else:
                print("Error occured " + e.strerror)

    return test_end_stub(instance, False)


def create_instance(bin_path, expected_path):
    while (True):
        with instance_mutex:
            if(active_instances >= MAX_INSTANCES):
                time.sleep(0.5)
            else:
                break

    instance = TestInstance(get_port(), bin_path, expected_path)
    instance.start()

    return instance

def do_tests():
    expected = get_all_files(EXPECTED_PATH)
    binaries = get_all_files(TEST_BINARY_PATH)

    argc = len(sys.argv)
    if (argc > 1):
        tested_binaries = []
        for i in range(1, argc):
            count = len(tested_binaries)
            for file in binaries:
                if (sys.argv[i] == file[1]):
                    tested_binaries.append(file)
                    break

            if (len(tested_binaries) == count):
                print(f"Failed to find {sys.argv[i]}")

        binaries = tested_binaries


    start = time.time()

    if (len(binaries) == 0):
        print("No binaries to test")
        return

    if (len(expected) == 0):
        print("No expecteds to test against")
        return

    instances = []

    for binary in binaries:
        equivalent = binary[1][:-4] + ".expect"
        expect = get_expected(expected, equivalent)

        if (expect[1] == ""):
            print(f"No expected found for test {binary[1]}")
        else:
            instances.append(create_instance(binary[0], expect[0]))


    active = MAX_INSTANCES
    while (active != 0):
        with instance_mutex:
            active = active_instances
        time.sleep(0.5)

    #ensure everything has ended, this can take some time
    for instance in instances:
        instance.end()

    print("All tests completed in " + str(time.time() - start) + " seconds")
    total_fail = 0
    total_pass = 0

    for instance in instances:
        result = instance.result
        if (result):
            total_pass += 1
            cprint(instance.bin_path.name + " PASSED", "light_green")
        else:
            total_fail += 1
            cprint(instance.bin_path.name + " FAILED", "red", attrs=["bold"])

    print("Summary: PASSED-{}, FAILED-{}, TOTAL-{}"
        .format(total_pass, total_fail, len(instances)))
    print("See .got files for more details")

if __name__ == "__main__":
    do_tests()