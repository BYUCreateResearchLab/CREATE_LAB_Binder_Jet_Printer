#include <Arduino.h>
#include <stdint.h>

enum class ERRORS
{
  NOERROR,
  PC_READ_ERROR,
  INVALID_COMMAND
};

constexpr uint8_t MAX_MESSAGE_LENGTH = 16;
char message[MAX_MESSAGE_LENGTH];     // a place to hold the incoming message

constexpr int LEFT_MISTER_PIN = A0;
constexpr int RIGHT_MISTER_PIN = A2;
ERRORS ERROR_STATE = ERRORS::NOERROR;

constexpr const char *ERROR_RESPONSE = "Error\n\r";
constexpr const char *INIT_RESPONSE = "MISTER\n\r";
constexpr const char *TIMEOUT_RESPONSE = "Timeout\n\r";

//=============================================================================
// utils
//=============================================================================

// possible commands the Arduino can respond to
// TODO add command to switch between using software serial / analog?
enum CMD
{
    INIT = 'Q',      // initialize controller
    MIST_ON = 'O',   // turn misters on
    MIST_OFF = 'C',  // turn misters off
    LMIST_ON = 'L',  // turn left mister on
    RMIST_ON = 'R'   // turn right mister on   
};

// NOTE must send this or ERROR_RESPONSE in response to all messages from PC
inline void okay_response()
{
    Serial.print("Placeholder");
    // maybe output the state of the two misters here
    Serial.print("\n\r"); // \r needed for PC to process message end
}

inline void respond()
{
    switch (ERROR_STATE)
    {
        case ERRORS::NOERROR: okay_response(); break;
        default: Serial.print(ERROR_RESPONSE); break;
    }

    ERROR_STATE = ERRORS::NOERROR; // reset error
}

inline void set_left_mister(bool state)
{
    digitalWrite(LEFT_MISTER_PIN, state);
}

inline void set_right_mister(bool state)
{
    digitalWrite(RIGHT_MISTER_PIN, state); 
}

inline void set_misters(bool state)
{
    set_left_mister(state);
    set_right_mister(state);
}

// decide what to do from received message
inline void handle_message()
{
    switch (message[0])
    {
    case INIT:     Serial.print(INIT_RESPONSE); return;
    case MIST_ON:  set_misters(true);      break;
    case MIST_OFF: set_misters(false);     break;
    case LMIST_ON: set_left_mister(true);  break;
    case RMIST_ON: set_right_mister(true); break;  
    default: ERROR_STATE = ERRORS::INVALID_COMMAND;
    }
    respond();
}

//=============================================================================
// arduino setup function (runs once at start of the program)
//=============================================================================
void setup()
{
    pinMode(LEFT_MISTER_PIN, OUTPUT);
    pinMode(RIGHT_MISTER_PIN, OUTPUT);
    set_misters(false);
    Serial.begin(19200); // baud rate used by Alicat controller
}

//=============================================================================
// arduino loop function (loops forever)
//=============================================================================
void loop()
{
    // check to see if anything is available in the serial receive buffer
    while (Serial.available())
    {
        // this is only called once and is not reset back to zero
        static unsigned int messagePos = 0;

        // read the next available byte in the serial receive buffer
        char inByte = Serial.read();
        if (inByte == '\n')
            continue; // ignore line-feed characters

        // add to message buffer if not the end
        if (inByte != '\r')
        {
            // add the incoming byte if there is still space in the buffer
            if (messagePos < MAX_MESSAGE_LENGTH - 1)
            {
                message[messagePos] = inByte;
                messagePos++;
                continue;
            }
            else // invalid command
            {
                message[MAX_MESSAGE_LENGTH - 1] = '\0'; // null-terminate
                messagePos = 0;
                ERROR_STATE = ERRORS::PC_READ_ERROR;
                respond();
                break;
            }
        }
        // terminating character received (full message received...)
        else
        {
            // add null character to string
            message[messagePos] = '\0';
            // Handle message if it exists
            if (messagePos > 0)
                handle_message();
            // reset for the next message
            messagePos = 0;
        }
    }
}
