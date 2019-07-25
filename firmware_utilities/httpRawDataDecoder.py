'''
Used for testing HTTP POST raw data functionality on the device.
'''
from flask import Flask, request
from struct import *

app = Flask(__name__)

@app.route('/')
def hello_world():
   return 'IU HTTP raw data decode test server'

# sizes of metadata in bytes
macId_size = 18
firmware_version_size = 8
timestamp_size = 8
blockSize_size = 4
sampling_rate_size = 4
axis_size = 1
total_metadata_size = macId_size + firmware_version_size + timestamp_size + blockSize_size + sampling_rate_size + axis_size



'''
Constructs byte decoding format string for IDE firmware v1.1.3. 
HTTP payload format :                       size in bytes
    char macId[18]                                  18
    char firmwareVersion[8]                         8
    double timestamp                                8
    int blockSize                                   4
    int samplingRate                                4
    char axis                                       1
    ---------------------------------------------------
    Total                                           43
    ---------------------------------------------------
    The following decoding is done by the caller of this function since blockSize has to
    be decoded first:
    q15_t rawValues[blockSize]                      2 * blockSize       
    (since 1 q15_t = 2 bytes so can be interpreted as short)                        
'''


def construct_metadata_format_string():
    f_string = '<'              # little endian
    f_string += 'c' * 18        # macId
    f_string += 'c' * 8         # firmwareVersion
    f_string += 'd'             # timestamp
    f_string += 'i'             # blockSize
    f_string += 'i'             # samplingRate
    f_string += 'c'             # axis
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

    scaling_factor = float(2 ** 13)
    g = 9.8

    f_string = construct_metadata_format_string()
    metadata = raw_payload[: total_metadata_size]
    decoded_metadata = unpack(f_string, metadata)

    mac_id = "".join(x.decode('utf-8') for x in decoded_metadata[0 : macId_size]).rstrip('\0')
    f_version = "".join([x.decode('utf-8') for x in decoded_metadata[macId_size : macId_size + firmware_version_size]]).rstrip('\0')
    timestamp = decoded_metadata[-4]
    block_size = decoded_metadata[-3]
    sampling_rate = decoded_metadata[-2]
    axis = decoded_metadata[-1].decode('utf-8')

    raw_data_format_string = 'h' * block_size
    decoded_raw_data = unpack(raw_data_format_string, raw_payload[total_metadata_size :])
    raw_values = list(map(float, decoded_raw_data))
    # required for fftProcessor script:
    raw_values = [x / scaling_factor for x in raw_values]
    raw_values = [x * g for x in raw_values]


    return {
        'raw_data'      : raw_values,
        'mac_id'        : mac_id,
        'f_version'     : f_version,
        'timestamp'     : timestamp,
        'block_size'    : block_size,
        'sampling_rate' : sampling_rate,
        'axis'          : axis
    }


'''
    Saves HTTP binary payload to file with appropriate 
    configuration as filename.

'''


def save_to_file(data, decoded):
    file_name = "block_size_" + str(decoded['block_size']) + "_sampling_rate_" + str(decoded['sampling_rate']) + "_axis_" + str(decoded['axis'])
    file = open(file_name, "wb")
    file.write(bytearray(data))
    file.close()


@app.route('/accel_raw_data', methods=["POST"])
def receive_accel_raw_data():
    data = request.get_data()
    decoded = get_payload_info(data)
    # save_to_file(data, decoded)

    # print("Raw binary data : ", data)
    print('Decoded data : ', decoded)
    print('Total size of payload data : ', len(data))
    print('Total size - metadata size : ', len(data) - total_metadata_size)
    return 'done'


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=1234)
