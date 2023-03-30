from threading import Thread, Lock
import os
import subprocess
import time
import socket
import errno


BASE_PORT = 1000
MAX_PORT = 1100

MAX_INSTANCES = 20

TEST_BINARY_PATH = "./src/"
EXPECTED_PATH = "./expected/"

LOCALHOST = "127.0.0.1"
DATA_DELAY = 0.1
SLICE_SIZE = 128

QEMU_ARGS = ["qemu-system-i386", "-boot", "c", "-drive", "format=raw,file=./../mOS.bin", "-no-reboot", "-no-shutdown", "-nographic", "-serial"]

if (os.name != "nt"):
    QEMU_ARGS = ["sudo"] + QEMU_ARGS

QEMU_SERIAL_DEV = "tcp:localhost:{port},server"

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

    def beginQemu(self):
        command = QEMU_ARGS.copy()
        command.append(QEMU_SERIAL_DEV.format(port=self.port))

        self._qemu = subprocess.Popen(command, stdin=subprocess.PIPE, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    
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
                    self._qemu.communicate(b"\x01x", 5)
                    self._qemu.wait(10)

                except:
                    # force qemu to quit since it refuses to exit normally
                    self._qemu.kill()

                self._qemu = None

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

# normal test exit
def test_end_stub(instance: TestInstance, result: bool):
    global active_instances, instance_mutex
    global used_ports, port_mutex

    instance.result = result

    with instance_mutex:
        active_instances -= 1

    instance.endQemu()

    with port_mutex:
        used_ports.remove(instance.port)


def test(instance: TestInstance):
    global active_instances, instance_mutex
    global used_ports, current_port, port_mutex

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            
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
                    

                else:
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

                    return test_end_stub(instance, True)

        except socket.timeout:
            print("failed to connect")

        except socket.error as e:
            
            if (e.errno == errno.EADDRINUSE):
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
    while (active_instances > MAX_INSTANCES):
        time.sleep(0.5)

    instance = TestInstance(get_port(), bin_path, expected_path)
    instance.start()
    
    return instance

def do_tests():
    expected = get_all_files(EXPECTED_PATH)
    binaries = get_all_files(TEST_BINARY_PATH)

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
            print("No expected found for test ${binary[1]}")
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

    print("All tests completed in " + str(time.time() - start) + "seconds")
    total_fail = 0
    total_pass = 0

    for instance in instances:
        result = instance.result
        if (result):
            total_pass += 1
            print(instance.bin_path.name + " PASSED")
        else:
            total_fail += 1
            print(instance.bin_path.name + " FAILED")

    print("Summary: PASSED-" + str(total_pass) + ", FAILED-" + str(total_fail) + ", TOTAL-" + str(len(instances)))
    print("See .got files for more details")

if __name__ == "__main__":
    do_tests()