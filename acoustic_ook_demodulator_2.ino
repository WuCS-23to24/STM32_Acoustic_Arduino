#include <analog.h>

uint16_t frame_data_decoded[100] = {};
const uint16_t frame_data[100] = {
    0xa5cf, 0x1f9e, 0xb63b, 0xec97, 0x99b4, 0x4860, 0x0e4f, 0x77b7, 0xc189, 0x6b16, 
    0xae15, 0x09cc, 0x2597, 0x3378, 0x5fd2, 0x843a, 0xd654, 0x5084, 0xbe5a, 0x74e1, 
    0x9ce7, 0x8df3, 0x7092, 0x3944, 0xfaf0, 0xc2a2, 0x07f1, 0x8d28, 0x033c, 0xc06c, 
    0xdbe1, 0xf9aa, 0x418e, 0x91a1, 0x783f, 0xb422, 0xb12d, 0x90fc, 0xa886, 0x62b3, 
    0x429f, 0x609c, 0xaf14, 0xcb1f, 0xef9b, 0x6aeb, 0xb0c7, 0x49cb, 0xcaf0, 0x4d56, 
    0xedbd, 0x864c, 0x09db, 0xd1a3, 0x0115, 0x4284, 0xaddf, 0x2c92, 0x8a32, 0x8697,   
    0xef94, 0xaf72, 0x48b8, 0x65b2, 0x02d6, 0xd6ef, 0xc927, 0x8203, 0x7169, 0xd30e,  
    0x3dae, 0x5953, 0x24de, 0x71b1, 0x448b, 0x4fc2, 0x9eba, 0xbe0f, 0xd5df, 0xb5ba, 
    0xe79f, 0x9512, 0x500c, 0x1983, 0x06eb, 0xc5ee, 0x3db6, 0x5b02, 0x33dd, 0x591b,  
    0xedb8, 0x5d73, 0xcf0c, 0x10c2, 0xcd97, 0x689d, 0xa059, 0x0055, 0xda40, 0xc05f
};

uint16_t getParity16(uint16_t n);

PinName adc_pin;

void setup()
{
    analogReadResolution(12);

    adc_pin = digitalPinToPinName(A0);
    dac_stop(adc_pin);

    Serial.begin(115200);
    Serial.println("Demodulator Board Setup Finished. Reading new frames in 10s.\n");

    delay(10000);
}

void loop()
{
    uint16_t frame_index = 0;
    uint16_t frame_parity_error_counter = 0;
    uint16_t frame_mismatch_counter = 0;

    uint16_t decoded_data = 0;
    uint16_t decode_bit_position = 0;

    const int symbol_period = 100;

    unsigned long time_start = 0;
    unsigned long time_end = 0;
    unsigned long time_delta = 0;

    float half_periods = 0.0;
    int half_periods_remaining = 0;
    
    bool start_seq_flag = 0;
    bool stop_seq_flag = 0;
    bool frame_successfully_decoded = 0;

    const int filter_size = 16;
    const int filter_threshold_upper = filter_size*1600;
    const int filter_threshold_lower = filter_size*1400;
    
    uint16_t filter[filter_size] = {0};
    uint32_t filter_sum = 0;
    uint16_t filter_output = 0;
    uint16_t filter_output_prev = 0;
    
    while(1)
    {
        // STEP 1: SAMPLE AND FILTER
        filter_sum = 0;
        for (int i = 0; i < filter_size-1; i++)
        {
            filter_sum += filter[i];
            filter[i] = filter[i+1];
        }
        filter[filter_size-1] = adc_read_value(adc_pin, 12);
        
        // STEP 2: EDGE DETECTION
        filter_output_prev = filter_output;
        if (filter_sum < filter_threshold_lower)
        {
            filter_output = 0;
        }
        else if (filter_sum > filter_threshold_upper )
        {
            filter_output = 1;
        }

        // STEP 3A: PROCESS RISING EDGE
        if (filter_output > filter_output_prev)
        {
            time_end = millis();
            time_delta = time_end-time_start;
            time_start = time_end;

            if (time_delta > 1000)
            {
                Serial.printf("\nNew sequence detected.\n");
            }
            else
            {
                half_periods = float(time_delta)/float(symbol_period/2); // consider changing to not require floats
                half_periods_remaining = round(half_periods);
                if (half_periods_remaining%2 == 1)
                {
                    half_periods_remaining--;
                    stop_seq_flag = 1; // the existence of the stop bit is determined here, but flag is used because 0s need to be counted before stopping decode
                }
                while (half_periods_remaining > 0)
                {
                    half_periods_remaining -= 2;
                    Serial.printf("0 ");
                    decode_bit_position++;
                }
                if (stop_seq_flag)
                {
                    Serial.printf("\nStop sequence detected.\n");
                    if (!start_seq_flag || decode_bit_position < 16)
                    {
                        Serial.printf("Stop sequence did not follow start sequence and 16 bits as expected. Frame discarded.\n");
                        decoded_data = 0;
                        decode_bit_position = 0;
                    }
                    else
                    {
                        frame_successfully_decoded = 1;
                    }
                    start_seq_flag = 0;
                }
            }
        }

        // STEP 3B: PROCESS FALLING EDGE
        else if (filter_output < filter_output_prev)
        {
            time_end = millis();
            time_delta = time_end-time_start;
            time_start = time_end;

            if (stop_seq_flag == 1)
            {
                stop_seq_flag = 0;
            }
            else
            {
                half_periods = float(time_delta)/float(symbol_period/2); // consider changing to not require floats
                half_periods_remaining = round(half_periods);
                if (half_periods_remaining%2 == 1)
                {
                    Serial.printf("Start sequence detected.\n");
                    start_seq_flag = 1;
                    half_periods_remaining--;
                    decoded_data = 0;
                    decode_bit_position = 0;
                }
                while (half_periods_remaining > 0)
                {
                    half_periods_remaining -= 2;
                    Serial.printf("1 ");
                    bitSet(decoded_data, decode_bit_position);
                    decode_bit_position++;
                }
            }  
        }

        // STEP 4: PROCESS DECODED FRAME
        if (frame_successfully_decoded && frame_index < 100)
        {
            frame_data_decoded[frame_index] = decoded_data;
            Serial.printf("Decoded frame %i. Frame data: 0x%08X. ");
            if (getParity16(frame_data_decoded[frame_index]))
            {
                Serial.printf("Parity incorrect. Error detected.\n");
                frame_parity_error_counter++;
            }
            else
            {
                Serial.printf("Parity correct.\n");
            }
            frame_index++;
        }

        // STEP 5: PROCESS DECODED MESSAGE 
        if (frame_index > 99)
        {
            Serial.printf("Sequence of 100 frames receieved. Writing to file and comparing to expected frame sequence.\n");
            // to do: write results to a file

            for (int i = 0; i < 100; i++)
            {
                if (frame_data_decoded[i] != frame_data[i])
                {
                    frame_mismatch_counter++;
                }
            } 
            Serial.printf("Frame parity errors detected: %i. Frame mismatch errors detected: %i.\n", frame_parity_error_counter, frame_mismatch_counter);
            frame_parity_error_counter = 0;
            frame_mismatch_counter = 0; 
            frame_index = 0;
        }
    }
}

uint16_t getParity16(uint16_t n) // returns 0x00 if even parity, 0x01 if odd parity
{
    uint16_t m = n ^ (n >> 1);
    m = m ^ (m >> 2);
    m = m ^ (m >> 4);
    m = m ^ (m >> 8);
    m = m & 0x0001;
    return m;
}

