'''
Used for testing HTTP POST raw data functionality on the device.
'''
from flask import Flask, request
from struct import *

app = Flask(__name__)

@app.route('/')
def hello_world():
   return 'IU HTTP raw data decode server'


'''
Constructs byte decoding format string for IDE firmware v1.1.3. See IUMessage.h for details.
HTTP payload format :                       size in bytes
    q15_t rawValues[maxBlockSize];              8192            q15_t is a 2 byte integer
    char macId[24];                             24 
    char firmwareVersion[8];                    8
    double timestamp;                           8
    int blockSize;                              4
    int samplingRate;                           4
    char axis;                                  1
    char padding[7];                            7
where maxBlockSize = 4096 and total size of payload = 8248 bytes
'''


def construct_format_string():
    f_string = '<' + 'h' * 4096     # little endian, rawValues
    f_string += 'c' * 24       # macId
    f_string += 'c' * 8        # firmwareVersion
    f_string += 'd'            # timestamp
    f_string += 'i'            # blockSize
    f_string += 'i'            # samplingRate
    f_string += 'c'            # axis
    f_string += 'x' * 7        # padding, not used
    return f_string


'''
Takes in raw HTTP payload and returns a dictionary of format : 
{
    'raw_data':         <float array> of size block_size
    'mac_id':           <String>,
    'f_version':        <String>,
    'timestamp':        <float>,   (UNIX epoch)
    'block_size':       <int>,
    'sampling_rate':    <int>,
    'axis':             <Single char string>,
}
'''


def get_payload_info(raw_payload):
    MAX_BLOCK_SIZE = 4096
    MAC_START = MAX_BLOCK_SIZE
    MAC_END = MAX_BLOCK_SIZE + 24
    F_VERSION_START = MAC_END
    F_VERSION_END = F_VERSION_START + 8
    SCALING_FACTOR = float(2 ** 13)
    G = 9.8

    f_string = construct_format_string()
    decoded = unpack(f_string, raw_payload)

    axis = decoded[-1].decode('utf-8')
    sampling_rate = int(decoded[-2])
    block_size = int(decoded[-3])
    timestamp = float(decoded[-4])
    mac_id = "".join([x.decode('utf-8') for x in decoded[MAC_START: MAC_END]]).rstrip('\0')     # remove padding bytes with rstrip()
    f_version = "".join([x.decode('utf-8') for x in decoded[F_VERSION_START: F_VERSION_END]]).rstrip('\0')
    all_values = list(map(float, list(decoded[:MAX_BLOCK_SIZE])))
    print("length : ", len(all_values))
    raw_values = list(map(float, list(decoded[:block_size])))
    raw_values = [x / SCALING_FACTOR for x in raw_values]                                       # not using numpy for minimal dependencies
    raw_values = [x * G for x in raw_values]

    return {
        'raw_data'      : raw_values,
        'mac_id'        : mac_id,
        'f_version'     : f_version,
        'timestamp'     : timestamp,
        'block_size'    : block_size,
        'sampling_rate' : sampling_rate,
        'axis'          : axis
    }


@app.route('/accel_raw_data', methods=["POST"])
def receive_accel_raw_data():
    data = request.get_data()
    decoded = get_payload_info(data)
    print('Decoded data : ', decoded)
    return 'done'


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=1234)
