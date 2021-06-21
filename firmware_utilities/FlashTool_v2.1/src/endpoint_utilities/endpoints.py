
import sys
import serial
import argparse
import time
import json

from utils.log import logger

class VersionNotAllowedError(BaseException):
    pass

class DeviceNotRespondingError(BaseException):
    pass

# *** Utility can work for firmware versions >= v1.1.3 ***
FIRMWARE_VERSION_FILEPATH = "../firmware_binaries/version.json"  # default path from root directory

# backends list will contain all the backends, the default ones and the custom ones defined in backends.json
backends = [
    {
        "key":"gcp",
        "backend":"GCP - v1.1.3 onwards",
        "mqtt":{"mqtt": {"mqttServerIP":"35.200.183.103","port":1883,"username":"ide_plus","password":"nW$Pg81o@EJD"}},
        "http":{"httpConfig":{"host":"ideplusrad-dot-infinite-uptime-1232.appspot.com","port":80,"path":"/rawaccelerationdata","username":"","password":"","oauth":""}}
    },
    {
        "key":"aws",
        "backend":"AWS - v1.1.3 onwards",
        "mqtt":{"mqtt": {"mqttServerIP":"13.233.38.155","port":1883,"username":"ispl","password":"indicus"}},
        "http":{"httpConfig":{"host":"13.232.122.10","port":8080,"path":"/iu-web/rawaccelerationdata","username":"","password":"","oauth":""}}
    },
    {
        "key":"awschina",  # NEED TO CHECK THE HTTP ENDPOINT FOR THIS ONE (is 'http://' required ??)
        "backend": "AWS-China - v1.1.3 onwards",
        "mqtt":{"mqtt": {"mqttServerIP":"52.83.251.81","port":1883,"username":"ispl","password":"indicus"}},
        "http":{"httpConfig":{"host":"52.83.190.160","port":8081,"path":"/iu-web/rawaccelerationdata","username":"","password":"","oauth":""}}
    },
    {
        "key":"uat",
        "backend":"UAT - v1.1.3 onwards",
        "mqtt":{"mqtt":{"mqttServerIP":"34.67.97.109","port":1883,"username":"iumqtt","password":"infiniteuptime@123"}},
        "http":{"httpConfig":{"host":"34.68.247.184","port":8080,"path":"/iu-web/rawaccelerationdata","username":"","password":"","oauth":""}}
    }
]


# Only configure MQTT and HTTP if detected version is >= 1.1.3., raise exceptions otherwise
def check_allowed_version(firmware_version_filepath):
    version = int(json.loads(open(firmware_version_filepath, "r").read())["version"].replace(".", ""))
    logger.info("BACKENDS - Detected version of firmware binaries: %s" % version)
    if (version < 113):
        raise VersionNotAllowedError

# Load the custom backends defined in backends.json
def load_custom_backends(backends_path):
    try:
        custom_backends = list(json.loads(open(backends_path, "r").read()))
        backends.extend(custom_backends)
        # Stop if any backend key has more than one definitions
        conflicting_backends = [backend["key"] for n, backend in enumerate(backends) if backend in backends[:n]]
        if conflicting_backends:
            logger.error("BACKENDS - Conflicting configurations found: {}".format(",".join(conflicting_backends)))
        else:
            logger.info("BACKENDS - Loaded custom backends from '{}' - {}".format(backends_path, ",".join([custom_backend["key"] for custom_backend in custom_backends])))
    except FileNotFoundError:
        pass
        # logger.error("BACKENDS - '{}' custom backends file not found".format(backends_path))

def get_serial_connection(device_path, timeout=None):
    serial_retries = 1
    number_of_retries = 10
    while True:
        try:
            logger.info("BACKENDS - Trying to establish serial connection to IDE... {}".format(serial_retries))
            ser = serial.Serial(device_path, 1152000, timeout=timeout)
            logger.info("BACKENDS - Serial connection to IDE established")
            return ser
        except serial.serialutil.SerialException as e:
            if serial_retries == number_of_retries:
                logger.error("BACKENDS - Serial connection with the device could not be established, failed to modify endpoints")
                return
            else:
                serial_retries += 1
                time.sleep(3)

IUGET_HTTP = "IUGET_HTTP_CONFIG"
IUGET_MQTT = "IUGET_MQTT_CONFIG"
IUGET_DEVICEID = "IUGET_DEVICEID"

# This blocks until a device is found at the other end of serial port
def get_device_response(serial_connection, command, lines=1):
    if send_device_command(serial_connection, command):
        response_lines = []
        logger.info("BACKENDS - Waiting for device to respond...")
        for i in range(lines):
            response = b""
            read_char = b""
            while read_char.decode("utf-8") != "\n":
                response += read_char
                read_char = serial_connection.read()
                if not read_char:
                    raise DeviceNotRespondingError
            response_lines.append(response[:-1].decode("utf-8"))
        return "\n".join(response_lines)
    else:
        return "No response received for {}".format(command)

def send_device_command(serial_connection, command):
    n = serial_connection.write((command+"\n").encode("utf-8"))
    if n == (len(command) + 1) : # count '\n'
        logger.info("BACKENDS - Command {} sent successfully".format(command))
        return True
    else:
        logger.fatal("BACKENDS - Command {} failed".format(command))
        return False

def wait_until_ide_boot(serial_connection):
    logger.info("BACKENDS - Fetching device MAC...")
    device_id_response = get_device_response(serial_connection, IUGET_DEVICEID)
    logger.info("BACKENDS - Detected device MAC: {}".format(device_id_response))

def set_endpoints(serial_connection, backend_key, mqtt=None, http=None):
    backend = [backend for backend in backends if backend["key"] == backend_key][0]
    logger.info("BACKENDS - UPDATING ENDPOINTS...")

    if mqtt:
        time.sleep(0.5)
        logger.info("BACKENDS - Updating MQTT endpoint to {}".format(backend["backend"]))
        send_device_command(serial_connection, json.dumps(backend["mqtt"]))

    if http:
        time.sleep(0.5)
        logger.info("BACKENDS - Updating HTTP endpoint to {}".format(backend["backend"]))
        send_device_command(serial_connection, json.dumps(backend["http"]))

def get_endpoints(serial_connection, mqtt=None, http=None):
    logger.info("BACKENDS - VERIFYING ENDPOINTS...")

    if mqtt:
        time.sleep(0.5)
        logger.info("BACKENDS - Checking current MQTT endpoint...")
        try:
            device_mqtt_response = get_device_response(serial_connection, IUGET_MQTT, 5)
            logger.info("BACKENDS - Current device MQTT endpoint: ")
            logger.info(device_mqtt_response)
        except DeviceNotRespondingError:
            logger.info("BACKENDS - Device does not contain MQTT configuration or FLASH memory is corrupted")

    if http:
        time.sleep(0.5)
        logger.info("BACKENDS - Checking current HTTP endpoint...")
        try:
            device_http_response = get_device_response(serial_connection, IUGET_HTTP, lines=4)
            logger.info("BACKENDS - Current device HTTP endpoint: ")
            logger.info(device_http_response)
        except DeviceNotRespondingError:
            logger.info("BACKENDS - Device does not contain HTTP configuration of FLASH memory is corrupted")


# MODIFYING ENDPOINTS
def main():
    global FIRMWARE_VERSION_FILEPATH

    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--operation",help="select operation", choices=["set", "get", "check", "show"], dest="operation", required=True)
    parser.add_argument("-cb", "--custom-backends", help="specify a json file of custom backends", dest="custom_backends")
    parser.add_argument("-b", "--backend", help="choose a backend", default=None, dest="backend")
    parser.add_argument("-d", "--device", help="specify device serial port", default="/dev/IDE", dest="device")
    parser.add_argument("--mqtt", help="set/get mqtt endpoint", action="store_true", default=False, dest="mqtt")
    parser.add_argument("--http", help="set/get http endpoint", action="store_true", default=False, dest="http")
    parser.add_argument("-fv", "--firmware-version", help="specify path to firmware version json", dest="firmware_version")

    if len(sys.argv)==1:
        parser.print_help()

    args = parser.parse_args()

    # Load the custom backends here
    if args.custom_backends:
        load_custom_backends(args.custom_backends)

    # If firmware version json filepath is specified, update the FIRMWARE_VERSION_FILEPATH var
    if args.firmware_version:
        FIRMWARE_VERSION_FILEPATH = args.firmware_version

    if args.operation == "set":
        try:
            check_allowed_version(FIRMWARE_VERSION_FILEPATH)
        except FileNotFoundError:
            print("BACKENDS - Firmware version could not be recognized, skipping backend configuration")
            sys.exit(0)
        except VersionNotAllowedError:
            print("BACKENDS - Firmware version is less than v1.1.3, skipping backend configuration")
            sys.exit(0)

        if not (args.mqtt or args.http):
            print("BACKENDS - Please specify '--http' or '--mqtt' to set endpoints")
        else:
            if (args.backend) in [backend["key"] for backend in backends]:
                serial_connection = get_serial_connection(args.device)
                wait_until_ide_boot(serial_connection)
                set_endpoints(serial_connection, args.backend, mqtt=args.mqtt, http=args.http)
                serial_connection.close()
            elif args.backend:
                print("BACKENDS - Incorrect backend platform  '{}' specified".format(args.backend))
                sys.exit(-1)
            else:
                print("BACKENDS - Please specify backend with '--backend / -b' flag")
                sys.exit(-1)

    elif args.operation == "get":
        try:
            check_allowed_version(FIRMWARE_VERSION_FILEPATH)
        except FileNotFoundError:
            print("BACKENDS - Firmware version could not be recognized, cannot fetch backend configuration")
            sys.exit(0)
        except VersionNotAllowedError:
            print("BACKENDS - Firmware version is less than v1.1.3, cannot fetch backend configuration")
            sys.exit(0)
        if not (args.mqtt or args.http):
            print("BACKENDS - Please specify '--http' or '--mqtt' to get endpoints")
        else:
            serial_connection = get_serial_connection(args.device)
            wait_until_ide_boot(serial_connection)  # block till device has booted up
            serial_connection.close()

            serial_connection = get_serial_connection(args.device, timeout=60)
            # timeout of 60 seconds, if device has not responded, indicates either missing config file or corrupted flash
            get_endpoints(serial_connection, mqtt=args.mqtt, http=args.http)
            serial_connection.close()

    elif args.operation == "show":
        for index, backend in enumerate(backends):
            print("BACKENDS - [{}] --> \t {}".format(backend["key"], backend["backend"]))

    elif args.operation == "check":                      # check if backend exists in (default+custom) backends
        if args.backend:
            if args.backend not in [backend["key"] for backend in backends]:
                print("BACKENDS - Specified backend is missing, exiting")
                sys.exit(-1)
            else:
                backend = [backend for backend in backends if backend["key"] == args.backend][0]
                print("BACKENDS - [{}] --> \t {}".format(backend["key"], backend["backend"]))
        else:
            print("BACKENDS - Please specify backend with '--backend / -b' flag")
            sys.exit(-1)

    sys.exit(0)

if __name__ == "__main__":
    main()
