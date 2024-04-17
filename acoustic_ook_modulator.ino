// On-Off Keying Test Code

void transmitFrame(uint16_t data, int carrier_frequency, int symbol_period);

void setup()
{
    pinMode(A0, OUTPUT);
    delay(5000);
}

void loop()
{
    uint16_t frame_data[100] = {
        0xa5cf, 0x1f9e, 0xb63b, 0xec97, 0x99b4, 0x4860, 0x0e4f, 0x77b7, 0xc189, 0x6b16,
        0xae15, 0x09cc, 0x2597, 0x3378, 0x5fd2, 0x843a, 0xd654, 0x5084, 0xbe5a, 0x74e1,
        0x9ce7, 0x8df3, 0x7092, 0x3944, 0xfaf0, 0xc2a2, 0x07f1, 0x8d28, 0x033c, 0xc06c,
        0xdbe1, 0xf9aa, 0x418e, 0x91a1, 0x783f, 0xb422, 0xb12d, 0x90fc, 0xa886, 0x62b3,
        0x429f, 0x609c, 0xaf14, 0xcb1f, 0xef9b, 0x6aeb, 0xb0c7, 0x49cb, 0xcaf0, 0x4d56,
        0xedbd, 0x864c, 0x09db, 0xd1a3, 0x0115, 0x4284, 0xaddf, 0x2c92, 0x8a32, 0x8697,
        0xef94, 0xaf72, 0x48b8, 0x65b2, 0x02d6, 0xd6ef, 0xc927, 0x8203, 0x7169, 0xd30e,
        0x3dae, 0x5953, 0x24de, 0x71b1, 0x448b, 0x4fc2, 0x9eba, 0xbe0f, 0xd5df, 0xb5ba,
        0xe79f, 0x9512, 0x500c, 0x1983, 0x06eb, 0xc5ee, 0x3db6, 0x5b02, 0x33dd, 0x591b, 
        0xedb8, 0x5d73, 0xcf0c, 0x10c2, 0xcd97, 0x689d, 0xa059, 0x0055, 0xda40, 0xc05f};

    const int carrier_frequency = 10000;
    const int symbol_period = 10;

    for (int i = 0; i < 100; i++)
    {
        transmitFrame(frame_data[i], carrier_frequency, symbol_period);
        delay(2000);
    }

    delay(100000);

}

void transmitFrame(uint16_t data, int carrier_frequency, int symbol_period)
{ 
    // START SEQUENCE: 1/2 period on, 1/2 period off
    tone(A0, carrier_frequency);
    delay(symbol_period/2);
    
    // DATA SEQUENCE: 1 period on for 1, 1 period off for 0 (NRZ)
    for (int i = 0; i < 16; i++)
    {
        if (bitRead(data, i) == 1)
        {
            tone(A0, carrier_frequency);
            delay(symbol_period);
        }
        else
        {
            noTone(A0);
            digitalWrite(A0, LOW);
            delay(symbol_period);
        }
    }
    
    // STOP SEQUENCE: 1/2 period off, 1/2 period on
    noTone(A0);
    delay(symbol_period/2);
    tone(A0, carrier_frequency);
    delay(symbol_period/2);
    noTone(A0);
    digitalWrite(A0, LOW);
}