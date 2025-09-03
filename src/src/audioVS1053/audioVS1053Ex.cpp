
/*****************************************************************************************
   vs1053_ext.cpp
 
   Created on: Jul 09.2017 
   Version 3.3.2r   Updated on: Jul 18.2025 (Maleksm)
   Author: Wolle (Dec 15.2023), easy
 ****************************************************************************************/
#include "../core/options.h"

#if I2S_DOUT==255
#include "../core/config.h"
#include "audioVS1053Ex.h"
//#include "../audioI2S/aac_decoder/aac_decoder.h"
#include "../audioI2S/flac_decoder/flac_decoder.h"
#include "../audioI2S/mp3_decoder/mp3_decoder.h"
//#include "../audioI2S/opus_decoder/opus_decoder.h"
#include "../audioI2S/vorbis_decoder/vorbis_decoder.h"
#include "../audioI2S/psram_unique_ptr.hpp"

/*#ifndef DMA_BUFCOUNT
  #define DMA_BUFCOUNT  16
#endif
#ifndef DMA_BUFLEN
  #define DMA_BUFLEN  512   //  (512)
#endif	*/

#ifndef VS_PATCH_ENABLE
  #define VS_PATCH_ENABLE  true
#endif

#define ANSI_ESC_RESET          "\033[0m"
#define ANSI_ESC_BLACK          "\033[30m"
#define ANSI_ESC_RED            "\033[31m"
#define ANSI_ESC_GREEN          "\033[32m"
#define ANSI_ESC_YELLOW         "\033[33m"
#define ANSI_ESC_BLUE           "\033[34m"
#define ANSI_ESC_MAGENTA        "\033[35m"
#define ANSI_ESC_CYAN           "\033[36m"
#define ANSI_ESC_WHITE          "\033[37m"

template <typename... Args>
void AUDIO_INFO(const char* fmt, Args&&... args) {
    ps_ptr<char> result;

    // First run: determine size
    int len = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
    if (len <= 0) return;

    result.alloc(len + 1, "result");
    char* dst = result.get();
    if (!dst) return;  // Or error treatment
    std::snprintf(dst, len + 1, fmt, std::forward<Args>(args)...);
//    result.append(ANSI_ESC_RESET);
    if(audio_info) audio_info(result.c_get());
    result.reset();
}
/*
template <typename... Args>
void AUDIO_ERROR_IMPL(const char* path, int line, const char* fmt, Args&&... args) {
    ps_ptr<char> result;
    ps_ptr<char> file;

    file.copy_from(path);
    while(file.contains("/")){
        file.remove_before('/', false);
    }

    // First run: determine size
    int len = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
    if (len <= 0) return;

    result.alloc(len + 1, "result");
    char* dst = result.get();
    if (!dst) return;
    std::snprintf(dst, len + 1, fmt, std::forward<Args>(args)...);

    // build a final string with file/line prefix
    ps_ptr<char> final;
    int total_len = std::snprintf(nullptr, 0, "%s:%d:" ANSI_ESC_RED " %s" ANSI_ESC_RESET, file.c_get(), line, dst);
    if (total_len <= 0) return;
    final.alloc(total_len + 1, "final");
    char* dest = final.get();
    if (!dest) return;  // or error treatment
    if(audio_info){
        std::snprintf(dest, total_len + 1, "%s:%d:" ANSI_ESC_RED " %s" ANSI_ESC_RESET, file.c_get(), line, dst);
        audio_info(final.get());
    }
    else{
         std::snprintf(dest, total_len + 1, "%s:%d: %s", file.c_get(), line, dst);
         log_e("%s", final.c_get());
    }
    final.reset();
    result.reset();
}	

// Macro for comfortable calls
#define AUDIO_ERROR(fmt, ...) AUDIO_ERROR_IMPL(__FILE__, __LINE__, fmt, ##__VA_ARGS__)	*/
template <typename... Args>
void AUDIO_ERROR(const char* fmt, Args&&... args) {
    ps_ptr<char> result;

    int len = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
    if (len <= 0) return;

    result.alloc(len + 1, "result");
    char* dst = result.get();
    if (!dst) return;
    std::snprintf(dst, len + 1, fmt, std::forward<Args>(args)...);
//    result.append(ANSI_ESC_RESET);
    audio_error(result.c_get());
    result.reset();
}

//###################################################################
AudioBuffer::AudioBuffer(size_t maxBlockSize) {
    if(maxBlockSize) m_resBuffSize = maxBlockSize;
    if(maxBlockSize) m_maxBlockSize = maxBlockSize;
}

AudioBuffer::~AudioBuffer() {
    if(m_buffer) free(m_buffer);
    m_buffer = NULL;
}

int32_t AudioBuffer::getBufsize() { return m_buffSize; }

bool AudioBuffer::setBufsize(size_t mbs) {
    if(mbs < 2 * m_resBuffSize) {
        AUDIO_ERROR("not allowed buffer size must be greater than %i", 2 * m_resBuffSize);
        return false;
    }
    m_buffSize = mbs;
    if(!init()) return false;
    return true;
}

size_t AudioBuffer::init() {
    if(m_buffer) free(m_buffer);
    m_buffer = NULL;
    m_buffer = (uint8_t*)ps_malloc(m_buffSize + m_resBuffSize);

    if(!m_buffer) return 0;
    m_f_init = true;
    resetBuffer();
    return m_buffSize;
}

void AudioBuffer::changeMaxBlockSize(uint16_t mbs) {
    m_maxBlockSize = mbs;
    return;
}

uint16_t AudioBuffer::getMaxBlockSize() { return m_maxBlockSize; }

size_t AudioBuffer::freeSpace() {
    if(m_readPtr == m_writePtr) {
        if(m_f_isEmpty == true) m_freeSpace = m_buffSize;
        else m_freeSpace = 0;
    }
    if(m_readPtr < m_writePtr) {
        m_freeSpace = (m_endPtr - m_writePtr) + (m_readPtr - m_buffer);
    }
    if(m_readPtr > m_writePtr) {
        m_freeSpace = m_readPtr - m_writePtr;
    }
    return m_freeSpace;
}

size_t AudioBuffer::writeSpace() {
    if(m_readPtr == m_writePtr) {
        if(m_f_isEmpty == true) m_writeSpace = m_endPtr - m_writePtr;
        else m_writeSpace = 0;
    }
    if(m_readPtr < m_writePtr) {
        m_writeSpace = m_endPtr - m_writePtr;
    }
    if(m_readPtr > m_writePtr) {
        m_writeSpace = m_readPtr - m_writePtr;
    }
    return m_writeSpace;
}

size_t AudioBuffer::bufferFilled() {
    if(m_readPtr == m_writePtr) {
        if(m_f_isEmpty == true) m_dataLength = 0;
        else m_dataLength = m_buffSize;
    }
    if(m_readPtr < m_writePtr) {
        m_dataLength = m_writePtr - m_readPtr;
    }
    if(m_readPtr > m_writePtr) {
        m_dataLength = (m_endPtr - m_readPtr) + (m_writePtr - m_buffer);
    }
    return m_dataLength;
}

size_t AudioBuffer::getMaxAvailableBytes() {
    if(m_readPtr == m_writePtr) {
    //   if(m_f_start)m_dataLength = 0;
        if(m_f_isEmpty == true) m_dataLength = 0;
        else m_dataLength = (m_endPtr - m_readPtr);
    }
    if(m_readPtr < m_writePtr) {
        m_dataLength = m_writePtr - m_readPtr;
    }
    if(m_readPtr > m_writePtr) {
        m_dataLength = (m_endPtr - m_readPtr);
    }
    return m_dataLength;
}

void AudioBuffer::bytesWritten(size_t bw) {
    if(!bw) return;
    m_writePtr += bw;
    if(m_writePtr == m_endPtr) { m_writePtr = m_buffer; }
    if(m_writePtr > m_endPtr) AUDIO_ERROR("AudioBuffer: m_writePtr %i > m_endPtr %i", m_writePtr, m_endPtr);
    m_f_isEmpty = false;
}

void AudioBuffer::bytesWasRead(size_t br) {
    if(!br) return;
    m_readPtr += br;
    if(m_readPtr >= m_endPtr) {
        size_t tmp = m_readPtr - m_endPtr;
        m_readPtr = m_buffer + tmp;
    }
    if(m_readPtr == m_writePtr) m_f_isEmpty = true;
}

uint8_t* AudioBuffer::getWritePtr() { return m_writePtr; }

uint8_t* AudioBuffer::getReadPtr() {
    int32_t len = m_endPtr - m_readPtr;
    if(len < m_maxBlockSize) {                            // be sure the last frame is completed
        memcpy(m_endPtr, m_buffer, m_maxBlockSize - (len)); // cpy from m_buffer to m_endPtr with len
    }
    return m_readPtr;
}

void AudioBuffer::resetBuffer() {
    m_writePtr = m_buffer;
    m_readPtr = m_buffer;
    m_endPtr = m_buffer + m_buffSize;
    m_f_isEmpty = true;
}

uint32_t AudioBuffer::getWritePos() { return m_writePtr - m_buffer; }

uint32_t AudioBuffer::getReadPos() { return m_readPtr - m_buffer; }
//###################################################################
// **** VS1053 Impl ****
//###################################################################
Audio::Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, SPIClass *spi) :
        cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin) {				// easy
//Audio::Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t spi, uint8_t mosi, uint8_t miso, uint8_t sclk) :    cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin) {					// Wolle
//Audio::Audio(uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin, uint8_t spi, uint8_t mosi, uint8_t miso, uint8_t sclk){

    cs_pin   = _cs_pin;
    dcs_pin  = _dcs_pin;
    dreq_pin = _dreq_pin;
//    mosi_pin = mosi;
//    miso_pin  = miso;
//    sclk_pin   = sclk;

    mutex_playAudioData = xSemaphoreCreateMutex();
    mutex_audioTask     = xSemaphoreCreateMutex();

#ifdef AUDIO_LOG
    m_f_Log = true;
#endif

    if(!psramFound()) AUDIO_ERROR("audioI2S requires PSRAM!");

//    spi_VS1053 = new SPIClass(spi);								// Wolle
    spi_VS1053 = spi; 											// easy
//    spi_VS1053->begin(sclk, miso, mosi, -1);							// Wolle
    spi_VS1053->begin(); 										// easy

    m_f_psramFound = psramInit();

    m_chbuf    = (char*) malloc(m_chbufSize);
    m_outBuff.alloc(m_outbuffSize * sizeof(int16_t), "m_outbuff");

    esp_err_t result = ESP_OK;

#if(ESP_ARDUINO_VERSION_MAJOR < 3)
    log_e("Arduino Version too old!");
#endif

    clientsecure.setInsecure();                 // update to ESP32 Arduino version 1.0.5-rc05 or higher
//    m_endFillByte=0;
//    curvol=50;
//    m_LFcount=0;

    startAudioTask();
}
//---------------------------------------------------------------------------------------------------------------------
Audio::~Audio(){
//    stopSong();
    setDefaults();

    stopAudioTask();
    vSemaphoreDelete(mutex_playAudioData);
    vSemaphoreDelete(mutex_audioTask);
}
//###################################################################
void Audio::initInBuff() {
    if(!InBuff.isInitialized()) {
        size_t size = InBuff.init();
        if(size > 0) {
            AUDIO_INFO("inputBufferSize: %u bytes", size - 1);
//            AUDIO_INFO("PSRAM %sfound, inputBufferSize: %u bytes", InBuff.havePSRAM() ? "" : "not ", size - 1);
        }
    }
    InBuff.changeMaxBlockSize(1600); // default size mp3 or aac
//    changeMaxBlockSize(4096);
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::control_mode_off(){
    CS_HIGH();                                     // End control mode
    spi_VS1053->endTransaction();                  // Allow other SPI users
}
void Audio::control_mode_on(){
    spi_VS1053->beginTransaction(VS1053_SPI_CTL);   // Prevent other SPI users
    DCS_HIGH();                                     // Bring slave in control mode
    CS_LOW();
}
void Audio::data_mode_on(){
    spi_VS1053->beginTransaction(VS1053_SPI_DATA);  // Prevent other SPI users
    CS_HIGH();                                      // Bring slave in data mode
    DCS_LOW();
}
void Audio::data_mode_off(){
    DCS_HIGH();
    spi_VS1053->endTransaction();                       // Allow other SPI users
}
//---------------------------------------------------------------------------------------------------------------------
uint16_t Audio::read_register(uint8_t _reg){
    uint16_t result=0;
    control_mode_on();
    spi_VS1053->write(3);                                           // Read operation
    spi_VS1053->write(_reg);                                        // Register to write (0..0xF)
    // Note: transfer16 does not seem to work
    result=(spi_VS1053->transfer(0xFF) << 8) | (spi_VS1053->transfer(0xFF));  // Read 16 bits data
    await_data_request();                                   // Wait for DREQ to be HIGH again
    control_mode_off();
    return result;
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::write_register(uint8_t _reg, uint16_t _value){
    control_mode_on();
    spi_VS1053->write(2);                                           // Write operation
    spi_VS1053->write(_reg);                                        // Register to write (0..0xF)
    spi_VS1053->write16(_value);                                    // Send 16 bits data
    await_data_request();
    control_mode_off();
}
//###################################################################
void Audio::sdi_send_buffer(uint8_t* data, size_t len) {
    size_t chunk_length;                                    // Length of chunk 32 byte or shorter

    data_mode_on();
    while(len){                                             // More to do?

        await_data_request();                               // Wait for space available
        chunk_length=len;
        if(len > vs1053_chunk_size){
            chunk_length=vs1053_chunk_size;
        }
        len-=chunk_length;
        spi_VS1053->writeBytes(data, chunk_length);
        data+=chunk_length;
    }
    data_mode_off();
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::sdi_send_fillers(size_t len){

    size_t chunk_length = 0;                                    // Length of chunk 32 byte or shorter

    data_mode_on();
    while(len) {                                             // More to do?
        await_data_request();                               // Wait for space available
        if(len > 0){ chunk_length = min((size_t)vs1053_chunk_size, len);  }
        len -= chunk_length;
        while(chunk_length--){ spi_VS1053->write(m_endFillByte); }
    }
    data_mode_off();
}
//---------------------------------------------------------------------------------------------------------------------
void Audio::wram_write(uint16_t address, uint16_t data){

    write_register(SCI_WRAMADDR, address);
    write_register(SCI_WRAM, data);
}
//---------------------------------------------------------------------------------------------------------------------
uint16_t Audio::wram_read(uint16_t address){

    write_register(SCI_WRAMADDR, address);                  // Start reading from WRAM
    return read_register(SCI_WRAM);                         // Read back result
}
//###################################################################
void Audio::begin(){

    pinMode(dreq_pin, INPUT);                               // DREQ is an input
    pinMode(cs_pin, OUTPUT);                                // The SCI and SDI signals
    pinMode(dcs_pin, OUTPUT);
    DCS_HIGH();
    CS_HIGH();
    delay(170);

    // Init SPI in slow mode ( 0.2 MHz )
    VS1053_SPI_CTL   = SPISettings( 200000, MSBFIRST, SPI_MODE0);
//    VS1053_SPI_DATA  = SPISettings(8000000, MSBFIRST, SPI_MODE0); // SPIDIV 10 -> 80/10=8.00 MHz	//800000
    delay(20);
//    VS1053_SPI._clock    = 200000;
//    VS1053_SPI._bitOrder = MSBFIRST;
//    VS1053_SPI._dataMode = SPI_MODE0;
    // printDetails("Right after reset/startup");

    // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
    // when playing MP3.  You can modify the board, but there is a more elegant way:
    wram_write(0xC017, 3);                                  // GPIO DDR=3
    wram_write(0xC019, 0);                                  // GPIO ODATA=0
    // printDetails("After test loop");
// - - - - - - - - - - - - - - - - - - - - - - // Do a soft reset - - - - - - - - - - - - - - - - - - - - - - - - -
    //softReset();                                            // Do a soft reset
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_RESET));
    delay(100);
    await_data_request();
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Switch on the analog parts
    write_register(SCI_AUDATA, 44100 + 1);                  // 44.1kHz + stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register(SCI_CLOCKF, 6 << 12);                    // Normal clock settings multiplyer 3.0=12.2 MHz
    VS1053_SPI_DATA  = SPISettings(6700000, MSBFIRST, SPI_MODE0); // SPIDIV 12 -> 80/12=6.66 MHz
//	    VS1053_SPI = SPISettings(6700000, MSBFIRST, SPI_MODE0); // SPIDIV 12 -> 80/12=6.66 MHz
    write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_LINE1));
    await_data_request();
 
    m_endFillByte = wram_read(0x1E06) & 0xFF;
    //  printDetails("After last clocksetting \n");

    if(VS_PATCH_ENABLE) loadUserCode(); // load patch in VS1053B if you want to play flac
    setVUmeter();

//    startSong();
    startAudioTask();
}
//###################################################################
/*//void Audio::softReset()
//{
//    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_RESET));
//    delay(100);
//    await_data_request();
//}
*/
//###################################################################
void Audio::setVUmeter() {

// * brief enable VSdsp VU Meter
// * param[in] enable when set will enable the VU meter
// * Writes the SS_VU_ENABLE bit of the SCI_STATUS register to enable VU meter on
// * board to the VSdsp.
// *
// * See data patches data sheet VU meter for details.
// * warning This feature is only available with patches that support VU meter.
// * n The VU meter takes about 0.2MHz of processing power with 48 kHz samplerate.

  if(!VS_PATCH_ENABLE) return;
  uint16_t VSstatus = read_register(SCI_STATUS);
  if(VSstatus==0) {
    Serial.println("VS1053 Error: Unable to write SCI_STATUS");
    _vuInitalized = false;
    return;
  }
  _vuInitalized = true;
  Serial.print("VS1053 status: OK! \t");
  write_register(SCI_STATUS, VSstatus | _BV(9));
}
//###################################################################
void Audio::loadUserCode(void) {
  int i = 0;

  while (i<sizeof(flac_plugin)/sizeof(flac_plugin[0])) {
    unsigned short addr, n, val;
    addr = flac_plugin[i++];
    n = flac_plugin[i++];
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = flac_plugin[i++];
      while (n--) {
        write_register(addr, val);
      }
    } else {           /* Copy run, copy n samples */
      while (n--) {
        val = flac_plugin[i++];
        write_register(addr, val);
      }
    }
  }
}
//###################################################################
void Audio::setDefaults() {
    stopSong();
    initInBuff(); // initialize InputBuffer if not already done
    InBuff.resetBuffer();
    m_outBuff.clear(); // Clear OutputBuffer
    vector_clear_and_shrink(m_playlistURL);
    vector_clear_and_shrink(m_playlistContent);
    m_hashQueue.clear();
    m_hashQueue.shrink_to_fit(); // uint32_t vector
    client.stop();
    clientsecure.stop();
    _client = static_cast<WiFiClient*>(&client); /* default to *something* so that no NULL deref can happen */
    ts_parsePacket(0, 0, 0);                     // reset ts routine
    m_lastM3U8host.reset();

    AUDIO_INFO("buffers freed, free Heap: %lu bytes", (long unsigned int)ESP.getFreeHeap());

    m_f_timeout = false;
    m_f_chunked = false; // Assume not chunked
    m_f_firstmetabyte = false;
    m_f_playing = false;
//    m_f_ssl = false;
    m_f_metadata = false;
    m_f_tts = false;
    m_f_firstCall = true;        // InitSequence for processWebstream and processLocalFile
    m_f_firstCurTimeCall = true; // InitSequence for computeAudioTime
    m_f_firstM3U8call = true;    // InitSequence for parsePlaylist_M3U8
    m_f_firstPlayCall = true;    // InitSequence for playAudioData
//    m_f_running = false;       // already done in stopSong
    m_f_unsync = false;   // set within ID3 tag but not used
    m_f_exthdr = false;   // ID3 extended header
    m_f_rtsp = false;     // RTSP (m3u8)stream
    m_f_m3u8data = false; // set again in processM3U8entries() if necessary
    m_f_continue = false;
    m_f_ts = false;
    m_f_ogg = false;
    m_f_m4aID3dataAreRead = false;
    m_f_stream = false;
    m_f_decode_ready = false;
    m_f_eof = false;
    m_f_ID3v1TagFound = false;
    m_f_lockInBuffer = false;
    m_f_acceptRanges = false;
    m_f_connectionClose = false;

    m_streamType = ST_NONE;
    m_codec = CODEC_NONE;
    m_playlistFormat = FORMAT_NONE;
    m_dataMode = AUDIO_NONE;
    m_streamTitle.assign("");
    m_resumeFilePos = -1;
    m_audioCurrentTime = 0; // Reset playtimer
    m_audioFileDuration = 0;
    m_audioDataStart = 0;
    m_audioDataSize = 0;
    m_localBitrateSend = false;
    m_avr_bitrate = 0;     // the same as m_bitrate if CBR, median if VBR
    m_bitRate = 0;         // Bitrate still unknown
    m_bytesNotDecoded = 0; // counts all not decodable bytes
    m_chunkcount = 0;      // for chunked streams
    m_contentlength = 0;   // If Content-Length is known, count it
//    m_curSample = 0;
    m_metaint = 0;        // No metaint yet
//    m_LFcount = 0;        // For end of header detection
    m_controlCounter = 0; // Status within readID3data() and readWaveHeader()
//    m_channels = 2;       // assume stereo #209
    m_fileSize = 0;
    m_ID3Size = 0;
    m_haveNewFilePos = 0;
    m_validSamples = 0;
//    m_M4A_chConfig = 0;
//    m_M4A_objectType = 0;
//    m_M4A_sampleRate = 0;
    m_sumBytesDecoded = 0;
//    m_opus_mode = 0;
    vuLeft = vuRight = 0; 		// #835

    if(m_f_reset_m3u8Codec){m_m3u8Codec = CODEC_AAC;} // reset to default
    m_f_reset_m3u8Codec = true;
}

//##################################################################
void Audio::setConnectionTimeout(uint16_t timeout_ms, uint16_t timeout_ms_ssl) {
    if(timeout_ms) m_timeout_ms = timeout_ms;
    if(timeout_ms_ssl) m_timeout_ms_ssl = timeout_ms_ssl;
}
//##################################################################
/*
    Text to speech API provides a speech endpoint based on our TTS (text-to-speech) model.
    More info: https://platform.openai.com/docs/guides/text-to-speech/text-to-speech

    Request body:
    model (string) [Required] - One of the available TTS models: tts-1 or tts-1-hd
    input (string) [Required] - The text to generate audio for. The maximum length is 4096 characters.
    instructions (string) [Optional] - A description of the desired characteristics of the generated audio.
    voice (string) [Required] - The voice to use when generating the audio. Supported voices are alloy, echo, fable, onyx, nova, and shimmer.
    response_format (string) [Optional] - Defaults to mp3. The format to audio in. Supported formats are mp3, opus, aac, and flac.
    speed (number) [Optional] - Defaults to 1. The speed of the generated audio. Select a value from 0.25 to 4.0. 1.0 is the default.

    Usage: audio.openai_speech(OPENAI_API_KEY, "tts-1", input, instructions, "shimmer", "mp3", "1");
*/
bool Audio::openai_speech(const String& api_key, const String& model, const String& input, const String& instructions, const String& voice, const String& response_format, const String& speed) {
    ps_ptr<char> host;
    host.assign("api.openai.com");
    char path[] = "/v1/audio/speech";

    if (input == "") {
        AUDIO_INFO("input text is empty");
        stopSong();
        return false;
    }
    xSemaphoreTakeRecursive(mutex_playAudioData, 0.3 * configTICK_RATE_HZ);

    setDefaults();
    m_f_ssl = true;

    m_speechtxt.assign(input.c_str());

    // Escape special characters in input
    String input_clean = "";
    for (int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        if (c == '\"') {
            input_clean += "\\\"";
        } else if (c == '\n') {
            input_clean += "\\n";
        } else if (c == '\r') {
            input_clean += "\\r";
        } else if (c == '\t') {
            input_clean += "\\t";
        } else if (c == '\\') {
            input_clean += "\\\\";
        } else if (c == '\b') {
            input_clean += "\\b";
        } else if (c == '\f') {
            input_clean += "\\f";
        } else {
            input_clean += c;
        }
    }

    // Escape special characters in instructions
    String instructions_clean = "";
    for (int i = 0; i < instructions.length(); i++) {
        char c = instructions.charAt(i);
        if (c == '\"') {
            instructions_clean += "\\\"";
        } else if (c == '\n') {
            instructions_clean += "\\n";
        } else if (c == '\r') {
            instructions_clean += "\\r";
        } else if (c == '\t') {
            instructions_clean += "\\t";
        } else if (c == '\\') {
            instructions_clean += "\\\\";
        } else if (c == '\b') {
            instructions_clean += "\\b";
        } else if (c == '\f') {
            instructions_clean += "\\f";
        } else {
            instructions_clean += c;
        }
    }

    String post_body = "{"
        "\"model\": \"" + model + "\"," +
        "\"stream\": true," +       // add
        "\"input\": \"" + input_clean + "\"," +
        "\"instructions\": \"" + instructions_clean + "\"," +
        "\"voice\": \"" + voice + "\"," +
        "\"response_format\": \"" + response_format + "\"," +
        "\"speed\": " + speed +
    "}";

    String http_request =
    //  "POST " + String(path) + " HTTP/1.0\r\n" // UNKNOWN ERROR CODE (0050) - crashing on HTTP/1.1 need to use HTTP/1.0
        "POST " + String(path) + " HTTP/1.1\r\n"
        + "Host: " + host.get() + "\r\n"
        + "Authorization: Bearer " + api_key + "\r\n"
        + "Accept-Encoding: identity;q=1,*;q=0\r\n"
        + "User-Agent: nArija/1.0\r\n"
        + "Content-Type: application/json; charset=utf-8\r\n"
        + "Content-Length: " + post_body.length() + "\r\n"
//        + "Connection: close\r\n" + "\r\n"
        + "\r\n"
        + post_body + "\r\n"
    ;

    bool res = true;
    int port = 443;
    _client = static_cast<WiFiClient*>(&clientsecure);

    uint32_t t = millis();
    AUDIO_INFO("Connect to: \"%s\"", host.get());
    res = _client->connect(host.get(), port, m_timeout_ms_ssl);
    if (res) {
        uint32_t dt = millis() - t;
        m_lastHost.assign(host.get());
        m_currentHost.clone_from(host);
        AUDIO_INFO("%s has been established in %lu ms", m_f_ssl ? "SSL" : "Connection", (long unsigned int)dt);
        m_f_running = true;
    }

    m_expectedCodec = CODEC_NONE;
    m_expectedPlsFmt = FORMAT_NONE;

    if (res) {
        _client->print(http_request);
        if (response_format == "mp3") m_expectedCodec  = CODEC_MP3;
        if (response_format == "opus") m_expectedCodec  = CODEC_OPUS;
        if (response_format == "aac") m_expectedCodec  = CODEC_AAC;
        if (response_format == "flac") m_expectedCodec  = CODEC_FLAC;

        m_dataMode = HTTP_RESPONSE_HEADER;
        m_f_tts = true;
    } else {
        AUDIO_INFO("Request %s failed!", host.get());
    }
    xSemaphoreGiveRecursive(mutex_playAudioData);
    return res;
}
//###################################################################
Audio::hwoe_t Audio::dismantle_host(const char* host){
    if (!host) return {};

    hwoe_t result;

    const char* p = host;

    // 🔐 1. SSL check
    if (strncmp(p, "https://", 8) == 0) {
        result.ssl = true;
        p += 8;
    } else if (strncmp(p, "http://", 7) == 0) {
        result.ssl = false;
        p += 7;
    } else {  // No valid scheme -> error or acceptance http
        result.ssl = false;
    }

    // ❓ 2. extract host (from p to ':' or '/' or '?')
    const char* host_start = p;
    const char* port_sep = strchr(p, ':');
    const char* path_sep = strchr(p, '/');
    const char* query_sep = strchr(p, '?');

    const char* host_end = p + strlen(p);  // default: end of string
    if (port_sep  && port_sep  < host_end) host_end = port_sep;
    if (path_sep  && path_sep  < host_end) host_end = path_sep;
    if (query_sep && query_sep < host_end) host_end = query_sep;
    result.hwoe.copy_from(host_start, host_end - host_start);

    // ❓ 3. extract port
    result.port = result.ssl ? 443 : 80; // default
    if (port_sep && (!path_sep || port_sep < path_sep)) {
        result.port = atoi(port_sep + 1);
    }

    // ❓ 4. extract extension (path)
    if (path_sep) {
        const char* path_start = path_sep + 1;
        const char* path_end = query_sep ? query_sep : host + strlen(host);
        result.extension.copy_from(path_start, path_end - path_start);
    }

    // ❓ 5. extract query string
    if (query_sep) {
        result.query_string.assign(query_sep + 1);
    }

    return result;
}
//###################################################################
bool Audio::connecttohost(const char* host, const char* user, const char* pwd) { // user and pwd for authentification only, can be empty

    if(!host) { AUDIO_INFO("Hostaddress is empty");     stopSong(); return false;}
    if (strlen(host) > 2048) { AUDIO_INFO("Hostaddress is too long");  stopSong(); return false;} // max length in Chrome DevTools

    bool     res           = false; // return value
    uint16_t port          = 0;     // port number
    uint16_t authLen       = 0;     // length of authorization
    uint32_t timestamp     = 0;     // timeout surveillance

    ps_ptr<char> c_host;       // copy of host
    ps_ptr<char> hwoe;         // host without extension
    ps_ptr<char> extension;    // extension
    ps_ptr<char> query_string; // parameter
    ps_ptr<char> path;         // extension + '?' + parameter
    ps_ptr<char> rqh;          // request header

    xSemaphoreTakeRecursive(mutex_playAudioData, 0.3 * configTICK_RATE_HZ);

    c_host.copy_from(host);
    c_host.trim();
    auto dismantledHost = dismantle_host(c_host.get());

//  https://edge.live.mp3.mdn.newmedia.nacamar.net:8000/ps-charivariwb/livestream.mp3;?user=ps-charivariwb;&pwd=ps-charivariwb-------
//      |   |                                     |    |                              |
//      |   |                                     |    |                              |             (query string)
//  ssl?|   |<-----host without extension-------->|port|<----- --extension----------->|<-first parameter->|<-second parameter->.......
//                                                     |
//                                                     |<-----------------------------path------------------------------------>......

    m_f_ssl = dismantledHost.ssl;
    port = dismantledHost.port;
    if(dismantledHost.hwoe.valid()) hwoe.clone_from(dismantledHost.hwoe);
    if(dismantledHost.extension.valid()) extension.clone_from(dismantledHost.extension);
    if(dismantledHost.query_string.valid()) query_string.clone_from(dismantledHost.query_string);

    if(extension.valid()) path.assign(extension.get());
    if(query_string.valid()){path.append("?"); path.append(query_string.get());}
    if(!hwoe.valid()) hwoe.assign("");
    if(!extension.valid()) extension.assign("");
    if(!path.valid()) path.assign("");

    path = urlencode(path.get(), true);

    // optional basic authorization
    if(user && pwd) authLen = strlen(user) + strlen(pwd);
    ps_ptr <char>authorization;
    ps_ptr<char> toEncode;
    authorization.alloc(base64_encode_expected_len(authLen + 1) + 1, "authorization");
    authorization.clear();
    if(authLen > 0) {
        toEncode.assign(user);
        toEncode.append(":");
        toEncode.append(pwd);
        b64encode((const char*)toEncode.get(), toEncode.strlen(), authorization.get());
    }

    setDefaults();
                       rqh.assign("GET /");
                       rqh.append(path.get());
                       rqh.append(" HTTP/1.1\r\n");
                       rqh.append("Host: ");
                       rqh.append(hwoe.get());
                       rqh.append("\r\n");
                       rqh.append("Icy-MetaData:1\r\n");
                       rqh.append("Icy-MetaData:2\r\n");
                       rqh.append("Pragma: no-cache\r\n");
                       rqh.append("Cache-Control: no-cache\r\n");
                       rqh.append("Accept:*/*\r\n");
                       rqh.append("User-Agent: VLC/3.0.21 LibVLC/3.0.21 AppleWebKit/537.36 (KHTML, like Gecko)\r\n");
    if(authLen > 0) {  rqh.append("Authorization: Basic ");
                       rqh.append(authorization.get());
                       rqh.append("\r\n"); }
                       rqh.append("Accept-Encoding: identity;q=1,*;q=0\r\n");
                       rqh.append("Connection: keep-alive\r\n\r\n");

    if(m_f_ssl) { _client = static_cast<WiFiClient*>(&clientsecure);}
    else        { _client = static_cast<WiFiClient*>(&client); }

    timestamp = millis();
    _client->setTimeout(m_f_ssl ? m_timeout_ms_ssl : m_timeout_ms);

    AUDIO_INFO("connect to: \"%s\" on port %d path \"/%s\"", hwoe.get(), port, path.get());
    res = _client->connect(hwoe.get(), port);

    m_expectedCodec = CODEC_NONE;
    m_expectedPlsFmt = FORMAT_NONE;

    if(res) {
        uint32_t dt = millis() - timestamp;
        AUDIO_INFO("%s has been established in %lu ms", m_f_ssl ? "SSL" : "Connection", (long unsigned int)dt);
        m_f_running = true;
        _client->print(rqh.get());
        if(extension.ends_with_icase( ".mp3" ))      m_expectedCodec  = CODEC_MP3;
        if(extension.ends_with_icase( ".aac" ))      m_expectedCodec  = CODEC_AAC;
        if(extension.ends_with_icase( ".aacp" )) m_expectedCodec  = CODEC_AAC;
        if(extension.ends_with_icase( ".wav" ))      m_expectedCodec  = CODEC_WAV;
        if(extension.ends_with_icase( ".m4a" ))      m_expectedCodec  = CODEC_M4A;
        if(extension.ends_with_icase( ".ogg" ))      m_expectedCodec  = CODEC_OGG;
        if(extension.ends_with_icase( ".vorbis"))   m_expectedCodec = CODEC_VORBIS;
        if(extension.ends_with_icase( "/vorbis"))   m_expectedCodec = CODEC_VORBIS;
        if(extension.ends_with_icase( ".flac"))      m_expectedCodec  = CODEC_FLAC;
        if(extension.ends_with_icase( "-flac"))      m_expectedCodec  = CODEC_FLAC;
        if(extension.ends_with_icase( ".opus"))      m_expectedCodec  = CODEC_OPUS;
        if(extension.ends_with_icase( "/opus"))      m_expectedCodec  = CODEC_OPUS;
        if(extension.ends_with_icase( ".asx" ))      m_expectedPlsFmt = FORMAT_ASX;
        if(extension.ends_with_icase( ".m3u" ))      m_expectedPlsFmt = FORMAT_M3U;
        if(extension.ends_with_icase( ".pls" ))      m_expectedPlsFmt = FORMAT_PLS;
        if(extension.contains(".m3u8"))              m_expectedPlsFmt = FORMAT_M3U8;

        m_currentHost.clone_from(c_host);
        m_lastHost.clone_from(c_host);
        if(audio_lasthost) audio_lasthost(m_lastHost.get());
        m_dataMode = HTTP_RESPONSE_HEADER; // Handle header
        m_streamType = ST_WEBSTREAM;
    }
    else {
        AUDIO_ERROR("Request %s failed!", c_host.get());
        m_f_running = false;
        if(audio_showstation) audio_showstation("");
        if(audio_showstreamtitle) audio_showstreamtitle("");
        if(audio_icydescription) audio_icydescription("");
        if(audio_icyurl) audio_icyurl("");
    }
    xSemaphoreGiveRecursive(mutex_playAudioData);
    return res;
}
//###################################################################
bool Audio::httpPrint(const char* host) {
    // user and pwd for authentification only, can be empty
    if(!m_f_running) return false;
    if(host == NULL) {AUDIO_ERROR("Hostaddress is empty"); stopSong(); return false;}

    uint16_t port = 0;         // port number
    ps_ptr<char> c_host;       // copy of host
    ps_ptr<char> hwoe;         // host without extension
    ps_ptr<char> extension;    // extension
    ps_ptr<char> query_string; // parameter
    ps_ptr<char> path;         // extension + '?' + parameter
    ps_ptr<char> rqh;          // request header
    ps_ptr<char> cur_hwoe;     // m_currenthost without extension

    c_host.copy_from(host);
    c_host.trim();
    auto dismantledHost = dismantle_host(c_host.get());

//  https://edge.live.mp3.mdn.newmedia.nacamar.net:8000/ps-charivariwb/livestream.mp3;?user=ps-charivariwb;&pwd=ps-charivariwb-------
//      |   |                                     |    |                              |
//      |   |                                     |    |                              |             (query string)
//  ssl?|   |<-----host without extension-------->|port|<----- --extension----------->|<-first parameter->|<-second parameter->.......
//                                                     |
//                                                     |<-----------------------------path------------------------------------------->

    m_f_ssl = dismantledHost.ssl;
    port = dismantledHost.port;
    if(dismantledHost.hwoe.valid()) hwoe.clone_from(dismantledHost.hwoe);
    if(dismantledHost.extension.valid()) extension.clone_from(dismantledHost.extension);
    if(dismantledHost.query_string.valid()) query_string.clone_from(dismantledHost.query_string);

    if(extension.valid()) path.assign(extension.get());
    if(query_string.valid()){path.append("?"); path.append(query_string.get());}
    if(!hwoe.valid()) hwoe.assign("");
    if(!extension.valid()) extension.assign("");
    if(!path.valid()) path.assign("");

    path = urlencode(path.get(), true);

    if(!m_currentHost.valid()) m_currentHost.assign("");
    auto dismantledLastHost = dismantle_host(m_currentHost.get());
    cur_hwoe.clone_from(dismantledLastHost.hwoe);

    bool f_equal = true;
    if(hwoe.equals(cur_hwoe)){f_equal = true;}
    else{                     f_equal = false;}

    rqh.assign("GET /");
    rqh.append(path.get());
    rqh.append(" HTTP/1.1\r\n");
    rqh.append("Host: ");
    rqh.append(hwoe.get());
    rqh.append("\r\n");
    rqh.append("Icy-MetaData:1\r\n");
    rqh.append("Icy-MetaData:2\r\n");
    rqh.append("Accept: */*\r\n");
    rqh.append("User-Agent: VLC/3.0.21 LibVLC/3.0.21 AppleWebKit/537.36 (KHTML, like Gecko)\r\n");
    rqh.append("Accept-Encoding: identity;q=1,*;q=0\r\n");
    rqh.append("Connection: keep-alive\r\n\r\n");

    AUDIO_INFO("next URL: \"%s\"", c_host.get());

    if(f_equal == false){
        _client->stop();
    }
    if(!_client->connected() ) {
         if(m_f_ssl) { _client = static_cast<WiFiClient*>(&clientsecure); if(m_f_ssl && port == 80) port = 443;}
         else        { _client = static_cast<WiFiClient*>(&client); }
        if(f_equal) AUDIO_INFO("The host has disconnected, reconnecting");

        if(!_client->connect(hwoe.get(), port)) {
            AUDIO_ERROR("connection lost %s", c_host.c_get());
            stopSong();
            return false;
        }
    }
    m_currentHost.clone_from(c_host);
    _client->print(rqh.get());

    if(     extension.ends_with_icase(".mp3"))       m_expectedCodec  = CODEC_MP3;
    else if(extension.ends_with_icase(".aac"))       m_expectedCodec  = CODEC_AAC;
    else if(extension.ends_with_icase(".aacp"))       m_expectedCodec  = CODEC_AAC;
    else if(extension.ends_with_icase(".wav"))       m_expectedCodec  = CODEC_WAV;
    else if(extension.ends_with_icase(".m4a"))       m_expectedCodec  = CODEC_M4A;
    else if(extension.ends_with_icase(".flac"))      m_expectedCodec  = CODEC_FLAC;
    else if(extension.ends_with_icase("-flac"))   m_expectedCodec = CODEC_FLAC;
    else if(extension.ends_with_icase(".vorbis"))   m_expectedCodec = CODEC_VORBIS;
    else if(extension.ends_with_icase("/vorbis"))   m_expectedCodec = CODEC_VORBIS;
    else if(extension.ends_with_icase(".opus"))   m_expectedCodec = CODEC_OPUS;
    else if(extension.ends_with_icase("/opus"))   m_expectedCodec = CODEC_OPUS;
    else if(extension.ends_with_icase(".ogg" ))   m_expectedCodec = CODEC_OGG;
    else if(extension.ends_with_icase("/ogg" ))   m_expectedCodec = CODEC_OGG;
    else                                       m_expectedCodec  = CODEC_NONE;

    if(     extension.ends_with_icase(".asx"))       m_expectedPlsFmt = FORMAT_ASX;
    else if(extension.ends_with_icase(".m3u"))       m_expectedPlsFmt = FORMAT_M3U;
    else if(extension.contains(".m3u8"))             m_expectedPlsFmt = FORMAT_M3U8;
    else if(extension.ends_with_icase(".pls"))       m_expectedPlsFmt = FORMAT_PLS;
    else                                             m_expectedPlsFmt = FORMAT_NONE;

    m_dataMode = HTTP_RESPONSE_HEADER; // Handle header
    m_streamType = ST_WEBSTREAM;
    m_contentlength = 0;
    m_f_chunked = false;

    return true;
}
//###################################################################
bool Audio::httpRange(const char* host, uint32_t range){    // user and pwd for authentification only, can be empty

    if(!m_f_running) return false;
    if(host == NULL) {
        AUDIO_INFO("Hostaddress is empty");
        stopSong();
        return false;
    }
    ps_ptr<char>c_host;
    c_host.copy_from(host);
    ps_ptr<char> h_host; // copy of host without http:// or https://

    if(startsWith(host, "https")) m_f_ssl = true;
    else m_f_ssl = false;

    if(m_f_ssl) h_host.append(host + 8);
    else        h_host.append(host + 7);

    int16_t  pos_slash;     // position of "/" in hostname
    int16_t  pos_colon;     // position of ":" in hostname
    int16_t  pos_ampersand; // position of "&" in hostname
    uint16_t port = 80;     // port number

    // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
    pos_slash = h_host.index_of( "/", 0);
    pos_colon = h_host.index_of( ":", 0);
    if(isalpha(h_host[pos_colon + 1])) pos_colon = -1; // no portnumber follows
    pos_ampersand = h_host.index_of( "&", 0);

    ps_ptr<char> hostwoext; // "skonto.ls.lv:8002" in "skonto.ls.lv:8002/mp3"
    ps_ptr<char> extension; // "/mp3" in "skonto.ls.lv:8002/mp3"

    if(pos_slash > 1) {
        hostwoext.alloc(pos_slash + 1, "hostwoext");
        memcpy(hostwoext.get(), h_host.get(), pos_slash);
        hostwoext[pos_slash] = '\0';
        extension = urlencode(h_host.get() + pos_slash, true);
    }
    else { // url has no extension
        hostwoext.append(h_host.get());
        extension.append("/");
    }

    if((pos_colon >= 0) && ((pos_ampersand == -1) || (pos_ampersand > pos_colon))) {
        port = atoi(h_host.get() + pos_colon + 1); // Get portnumber as integer
        hostwoext[pos_colon] = '\0';         // Host without portnumber
    }

    char rqh[strlen(h_host.get()) + strlen(host) + 300]; // http request header
    rqh[0] = '\0';
    char ch_range[12];
    ltoa(range, ch_range, 10);
    AUDIO_INFO("skip to position: %li", (long int)range);
    strcat(rqh, "GET ");
    strcat(rqh, extension.get());
    strcat(rqh, " HTTP/1.1\r\n");
    strcat(rqh, "Host: ");
    strcat(rqh, hostwoext.get());
    strcat(rqh, "\r\n");
    strcat(rqh, "Icy-MetaData:1\r\n");
    strcat(rqh, "Icy-MetaData:2\r\n");
    strcat(rqh, "Range: bytes=");
    strcat(rqh, (const char*)ch_range);
    strcat(rqh, "-\r\n");
    strcat(rqh, "Referer: ");
    strcat(rqh, host);
    strcat(rqh, "\r\n");
    strcat(rqh, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/129.0.0.0 Safari/537.36\r\n");
    strcat(rqh, "Connection: keep-alive\r\n\r\n");

    _client->stop();
    if(m_f_ssl) { _client = static_cast<WiFiClient*>(&clientsecure); if(m_f_ssl && port == 80) port = 443;}
    else        { _client = static_cast<WiFiClient*>(&client); }
    AUDIO_INFO("The host has disconnected, reconnecting");
    if(!_client->connect(hostwoext.get(), port)) {
        AUDIO_ERROR("connection lost %s", c_host.c_get());
        stopSong();
        return false;
    }
    _client->print(rqh);
    if(extension.ends_with_icase(".mp3"))       m_expectedCodec  = CODEC_MP3;
    if(extension.ends_with_icase(".aac"))       m_expectedCodec  = CODEC_AAC;
    if(extension.ends_with_icase(".wav"))       m_expectedCodec  = CODEC_WAV;
    if(extension.ends_with_icase(".m4a"))       m_expectedCodec  = CODEC_M4A;
    if(extension.ends_with_icase(".flac"))      m_expectedCodec  = CODEC_FLAC;
    if(extension.ends_with_icase(".opus"))      m_expectedCodec  = CODEC_OPUS;
    if(extension.ends_with_icase(".vorbis"))      m_expectedCodec  = CODEC_VORBIS;
    if(extension.ends_with_icase(".ogg"))      m_expectedCodec  = CODEC_OGG;
    if(extension.ends_with_icase(".asx"))       m_expectedPlsFmt = FORMAT_ASX;
    if(extension.ends_with_icase(".m3u"))       m_expectedPlsFmt = FORMAT_M3U;
    if(extension.contains(".m3u8"))             m_expectedPlsFmt = FORMAT_M3U8;
    if(extension.ends_with_icase(".pls"))       m_expectedPlsFmt = FORMAT_PLS;

    m_dataMode = HTTP_RESPONSE_HEADER; // Handle header
    m_streamType = ST_WEBFILE;
    m_contentlength = 0;
    m_f_chunked = false;

    return true;
}
//###################################################################
bool Audio::connecttoSD(String sdfile, int32_t fileStartPos) {
    return connecttoFS(SD, sdfile.c_str(), fileStartPos); }

bool Audio::connecttoSD(const char* path, int32_t fileStartPos) {
    return connecttoFS(SD, path, fileStartPos); }
//###################################################################
bool Audio::connecttoFS(fs::FS& fs, const char* path, int32_t fileStartPos) {

    xSemaphoreTakeRecursive(mutex_playAudioData, 0.3 * configTICK_RATE_HZ);
    ps_ptr<char>c_path;
//    ps_ptr<char> audioPath;
    bool res = false;
    m_fileStartPos = fileStartPos;
    uint8_t codec = CODEC_NONE;

    if(!path) {AUDIO_INFO("file path is not set"); goto exit;}  // guard
    c_path.copy_from(path); // copy from path
    c_path.trim();
    if(!c_path.contains(".")) {AUDIO_INFO("No file extension found"); goto exit;}  // guard
    setDefaults(); // free buffers an set defaults

    if(c_path.ends_with_icase(".mp3")) { codec = CODEC_MP3; res = true; audio_info("format is mp3"); }
//    if(c_path.ends_with_icase(".m4a")) { codec = CODEC_M4A; res = true; audio_info("format is aac"); }
//    if(c_path.ends_with_icase(".aac")) { codec = CODEC_AAC; res = true; audio_info("format is aac"); }
//    if(c_path.ends_with_icase(".wav")) { codec = CODEC_WAV; res = true; audio_info("format is wav"); }
    if(c_path.ends_with_icase(".flac")) { codec = CODEC_FLAC; res = true; audio_info("format is flac"); }
//    if(c_path.ends_with_icase(".opus")) { codec = CODEC_OPUS; res = true; m_f_ogg = true; audio_info("format is opus"); }
//    if(c_path.ends_with_icase(".ogg")) { codec = CODEC_OGG; res = true; m_f_ogg = true; audio_info("format is ogg"); }
//    if(c_path.ends_with_icase(".oga")) { codec = CODEC_OGG; res = true; m_f_ogg = true; audio_info("format is ogg"); }
    if(codec == CODEC_NONE) {   // guard
        uint16_t dotPos = c_path.last_index_of('.');
        AUDIO_ERROR("The %s format is not supported", path + dotPos); goto exit;
    }

    if(!c_path.starts_with("/")) c_path.insert("/", 0);

    if(!fs.exists(c_path.get())) {AUDIO_INFO("file not found: %s", c_path.get()); goto exit;}
    AUDIO_INFO("Reading file: \"%s\"", c_path.get());
    m_audiofile = fs.open(c_path.get());
    m_dataMode = AUDIO_LOCALFILE;
    m_fileSize = m_audiofile.size();

//    res = initializeDecoder(codec);
    m_codec = codec;
    if(res) m_f_running = true;
    else m_audiofile.close();

exit:
    xSemaphoreGiveRecursive(mutex_playAudioData);
    return res;
}
//###################################################################
/*
 void Audio::printProcessLog(int r, const char* s){
    const char* e;
    const char* f = "";
    uint8_t logLevel;  // 1 Error, 2 Warn, 3 Info,
    switch(r) {
        case AUDIOLOG_PATH_IS_NULL: e = "The path or file name is empty"; logLevel = 1; break;
        case AUDIOLOG_OUT_OF_MEMORY: e = "Out of memory"; logLevel = 1; break;
        case AUDIOLOG_FILE_NOT_FOUND: e = "File doesn't exist: "; logLevel = 1; f = s; break;
        case AUDIOLOG_FILE_READ_ERR: e = "Failed to open file for reading"; logLevel = 1; break;

        default: e = "UNKNOWN EVENT"; logLevel = 3; break;
    }
    if(audio_log){
        audio_log(logLevel, e, f);
    }
    else {
        if     (logLevel == 1) {AUDIO_INFO("ERROR: %s%s", e, f);}
        else if(logLevel == 2) {AUDIO_INFO("WARNING: %s%s", e, f);}
        else                   {AUDIO_INFO("INFO: %s%s", e, f);}
    }
 }	*/
//###################################################################
bool Audio::connecttospeech(const char* speech, const char* lang) {
    xSemaphoreTakeRecursive(mutex_playAudioData, 0.3 * configTICK_RATE_HZ);

    setDefaults();
    char host[] = "translate.google.com.vn";
    char path[] = "/translate_tts";

    m_speechtxt.assign(speech); // unique pointer takes care of the memory management
    auto urlStr = urlencode(speech, false); // percent encoding

    ps_ptr<char> req; // request header
    req.assign("GET ");
    req.append(path);
    req.append("?ie=UTF-8&tl=");
    req.append(lang);
    req.append("&client=tw-ob&q=");
    req.append(urlStr.get());
    req.append(" HTTP/1.1\r\n");
    req.append("Host: ");
    req.append(host);
    req.append("\r\n");
    req.append("User-Agent: Mozilla/5.0 \r\n");
    req.append("Accept-Encoding: identity\r\n");
    req.append("Accept: text/html\r\n");
    req.append("Connection: close\r\n\r\n");

    _client = static_cast<WiFiClient*>(&client);
    AUDIO_INFO("connect to: \"%s\"", host);
    if(!_client->connect(host, 80)) {
        AUDIO_ERROR("Connection failed");
        xSemaphoreGiveRecursive(mutex_playAudioData);
        return false;
    }
    _client->print(req.get());

    m_f_running = true;
    m_f_ssl = false;
    m_f_tts = true;
    m_dataMode = HTTP_RESPONSE_HEADER;
    m_lastHost.assign(host);
    m_currentHost.copy_from(host);
    xSemaphoreGiveRecursive(mutex_playAudioData);
    return true;
}
//###################################################################
void Audio::showID3Tag(const char* tag, const char* value) {
    ps_ptr<char>id3tag = {};
    // V2.2
    if(!strcmp(tag, "CNT")) id3tag.appendf("Play counter: %s", value, "id3tag");
    if(!strcmp(tag, "COM")) id3tag.appendf("Comments: %s", value, "id3tag");
    if(!strcmp(tag, "CRA")) id3tag.appendf("Audio encryption: %s", value, "id3tag");
    if(!strcmp(tag, "CRM")) id3tag.appendf("Encrypted meta frame: %s", value, "id3tag");
    if(!strcmp(tag, "ETC")) id3tag.appendf("Event timing codes: %s", value, "id3tag");
    if(!strcmp(tag, "EQU")) id3tag.appendf("Equalization: %s", value, "id3tag");
    if(!strcmp(tag, "IPL")) id3tag.appendf("Involved people list: %s", value, "id3tag");
    if(!strcmp(tag, "PIC")) id3tag.appendf("Attached picture: %s", value, "id3tag");
    if(!strcmp(tag, "SLT")) id3tag.appendf("Synchronized lyric/text: %s", value, "id3tag");
    if(!strcmp(tag, "TAL")) { id3tag.appendf("Album/Movie/Show title: %s", value, "id3tag"); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TBP")) id3tag.appendf("BPM (Beats Per Minute): %s", value, "id3tag");
    if(!strcmp(tag, "TCM")) id3tag.appendf("Composer: %s", value, "id3tag");
    if(!strcmp(tag, "TCO")) id3tag.appendf("Content type: %s", value, "id3tag");
    if(!strcmp(tag, "TCR")) id3tag.appendf("Copyright message: %s", value, "id3tag");
    if(!strcmp(tag, "TDA")) id3tag.appendf("Date: %s", value, "id3tag");
    if(!strcmp(tag, "TDY")) id3tag.appendf("Playlist delay: %s", value, "id3tag");
    if(!strcmp(tag, "TEN")) id3tag.appendf("Encoded by: %s", value, "id3tag");
    if(!strcmp(tag, "TFT")) id3tag.appendf("File type: %s", value, "id3tag");
    if(!strcmp(tag, "TIM")) id3tag.appendf("Time: %s", value, "id3tag");
    if(!strcmp(tag, "TKE")) id3tag.appendf("Initial key: %s", value, "id3tag");
    if(!strcmp(tag, "TLA")) id3tag.appendf("Language(s): %s", value, "id3tag");
    if(!strcmp(tag, "TLE")) id3tag.appendf("Length: %s", value, "id3tag");
    if(!strcmp(tag, "TMT")) id3tag.appendf("Media type: %s", value, "id3tag");
    if(!strcmp(tag, "TOA")) { id3tag.appendf("Original artist(s)/performer(s): %s", value, "id3tag"); if(audio_id3artist) audio_id3artist(value); }
    if(!strcmp(tag, "TOF")) id3tag.appendf("Original filename: %s", value, "id3tag");
    if(!strcmp(tag, "TOL")) id3tag.appendf("Original Lyricist(s)/text writer(s): %s", value, "id3tag");
    if(!strcmp(tag, "TOR")) id3tag.appendf("Original release year: %s", value, "id3tag");
    if(!strcmp(tag, "TOT")) id3tag.appendf("Original album/Movie/Show title: %s", value, "id3tag");
    if(!strcmp(tag, "TP1")) id3tag.appendf("Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group: %s", value, "id3tag");
    if(!strcmp(tag, "TP2")) id3tag.appendf("Band/Orchestra/Accompaniment: %s", value, "id3tag");
    if(!strcmp(tag, "TP3")) id3tag.appendf("Conductor/Performer refinement: %s", value, "id3tag");
    if(!strcmp(tag, "TP4")) id3tag.appendf("Interpreted, remixed, or otherwise modified by: %s", value, "id3tag");
    if(!strcmp(tag, "TPA")) id3tag.appendf("Part of a set: %s", value, "id3tag");
    if(!strcmp(tag, "TPB")) id3tag.appendf("Publisher: %s", value, "id3tag");
    if(!strcmp(tag, "TRC")) id3tag.appendf("ISRC (International Standard Recording Code): %s", value, "id3tag");
    if(!strcmp(tag, "TRD")) id3tag.appendf("Recording dates: %s", value, "id3tag");
    if(!strcmp(tag, "TRK")) id3tag.appendf("Track number/Position in set: %s", value, "id3tag");
    if(!strcmp(tag, "TSI")) id3tag.appendf("Size: %s", value, "id3tag");
    if(!strcmp(tag, "TSS")) id3tag.appendf("Software/hardware and settings used for encoding: %s", value, "id3tag");
    if(!strcmp(tag, "TT1")) id3tag.appendf("Content group description: %s", value, "id3tag");
    if(!strcmp(tag, "TT2")) { id3tag.appendf("Title/Songname/Content description: %s", value, "id3tag"); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TT3")) id3tag.appendf("Subtitle/Description refinement: %s", value, "id3tag");
    if(!strcmp(tag, "TXT")) id3tag.appendf("Lyricist/text writer: %s", value, "id3tag");
    if(!strcmp(tag, "TXX")) id3tag.appendf("User defined text information frame: %s", value, "id3tag");
    if(!strcmp(tag, "TYE")) id3tag.appendf("Year: %s", value, "id3tag");
    if(!strcmp(tag, "UFI")) id3tag.appendf("Unique file identifier: %s", value, "id3tag");
    if(!strcmp(tag, "ULT")) id3tag.appendf("Unsychronized lyric/text transcription: %s", value, "id3tag");
    if(!strcmp(tag, "WAF")) id3tag.appendf("Official audio file webpage: %s", value, "id3tag");
    if(!strcmp(tag, "WAR")) id3tag.appendf("Official artist/performer webpage: %s", value, "id3tag");
    if(!strcmp(tag, "WAS")) id3tag.appendf("Official audio source webpage: %s", value, "id3tag");
    if(!strcmp(tag, "WCM")) id3tag.appendf("Commercial information: %s", value, "id3tag");
    if(!strcmp(tag, "WCP")) id3tag.appendf("Copyright/Legal information: %s", value, "id3tag");
    if(!strcmp(tag, "WPB")) id3tag.appendf("Publishers official webpage: %s", value, "id3tag");
    if(!strcmp(tag, "WXX")) id3tag.appendf("User defined URL link frame: %s", value, "id3tag");

    // V2.3 V2.4 tags
    if(!strcmp(tag, "COMM")) id3tag.appendf("Comment: %s", value, "id3tag");
    if(!strcmp(tag, "OWNE")) id3tag.appendf("Ownership: %s", value, "id3tag");
    if(!strcmp(tag, "PRIV")) id3tag.appendf("Private: %s", value, "id3tag");
    if(!strcmp(tag, "SYLT")) id3tag.appendf("SynLyrics: %s", value, "id3tag");
    if(!strcmp(tag, "TALB")) { id3tag.appendf("Album: %s", value, "id3tag"); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TBPM")) id3tag.appendf("BeatsPerMinute: %s", value, "id3tag");
    if(!strcmp(tag, "TCMP")) id3tag.appendf("Compilation: %s", value, "id3tag");
    if(!strcmp(tag, "TCOM")) id3tag.appendf("Composer: %s", value, "id3tag");
    if(!strcmp(tag, "TCON")) id3tag.appendf("ContentType: %s", value, "id3tag");
    if(!strcmp(tag, "TCOP")) id3tag.appendf("Copyright: %s", value, "id3tag");
    if(!strcmp(tag, "TDAT")) id3tag.appendf("Date: %s", value, "id3tag");
    if(!strcmp(tag, "TDOR")) id3tag.appendf("Original Release Year: %s", value, "id3tag");
    if(!strcmp(tag, "TDRC")) id3tag.appendf("Publication date: %s", value, "id3tag");
    if(!strcmp(tag, "TEXT")) id3tag.appendf("Lyricist: %s", value, "id3tag");
    if(!strcmp(tag, "TIME")) id3tag.appendf("Time: %s", value, "id3tag");
    if(!strcmp(tag, "TIT1")) id3tag.appendf("Grouping: %s", value, "id3tag");
    if(!strcmp(tag, "TIT2")) { id3tag.appendf("Title: %s", value, "id3tag"); if(audio_id3album) audio_id3album(value); }
    if(!strcmp(tag, "TIT3")) id3tag.appendf("Subtitle: %s", value, "id3tag");
    if(!strcmp(tag, "TLAN")) id3tag.appendf("Language: %s", value, "id3tag");
    if(!strcmp(tag, "TLEN")) id3tag.appendf("Length (ms): %s", value, "id3tag");
    if(!strcmp(tag, "TMED")) id3tag.appendf("Media: %s", value, "id3tag");
    if(!strcmp(tag, "TOAL")) id3tag.appendf("OriginalAlbum: %s", value, "id3tag");
    if(!strcmp(tag, "TOPE")) id3tag.appendf("OriginalArtist: %s", value, "id3tag");
    if(!strcmp(tag, "TORY")) id3tag.appendf("OriginalReleaseYear: %s", value, "id3tag");
    if(!strcmp(tag, "TPE1")) { id3tag.appendf("Artist: %s", value, "id3tag"); if(audio_id3artist) audio_id3artist(value); }
    if(!strcmp(tag, "TPE2")) id3tag.appendf("Band: %s", value, "id3tag");
    if(!strcmp(tag, "TPE3")) id3tag.appendf("Conductor: %s", value, "id3tag");
    if(!strcmp(tag, "TPE4")) id3tag.appendf("InterpretedBy: %s", value, "id3tag");
    if(!strcmp(tag, "TPOS")) id3tag.appendf("PartOfSet: %s", value, "id3tag");
    if(!strcmp(tag, "TPUB")) id3tag.appendf("Publisher: %s", value, "id3tag");
    if(!strcmp(tag, "TRCK")) id3tag.appendf("Track: %s", value, "id3tag");
    if(!strcmp(tag, "TSSE")) id3tag.appendf("SettingsForEncoding: %s", value, "id3tag");
    if(!strcmp(tag, "TRDA")) id3tag.appendf("RecordingDates: %s", value, "id3tag");
    if(!strcmp(tag, "TXXX")) id3tag.appendf("UserDefinedText: %s", value, "id3tag");
    if(!strcmp(tag, "TYER")) id3tag.appendf("Year: %s", value, "id3tag");
    if(!strcmp(tag, "USER")) id3tag.appendf("TermsOfUse: %s", value, "id3tag");
    if(!strcmp(tag, "USLT")) id3tag.appendf("Lyrics: %s", value, "id3tag");
    if(!strcmp(tag, "WOAR")) id3tag.appendf("OfficialArtistWebpage: %s", value, "id3tag");
    if(!strcmp(tag, "WXXX")) id3tag.appendf("User defined URL link frame: %s", value, "id3tag");
    if(!strcmp(tag, "XDOR")) id3tag.appendf("OriginalReleaseTime: %s", value, "id3tag");

    if(!id3tag.valid()){log_w("tag not found: %s", tag); return;}
    latinToUTF8(id3tag);
    if(id3tag.contains("?xml")) {
        showstreamtitle(id3tag.get());
        return;
    }
    if(id3tag.strlen()) {
        if(audio_id3data) audio_id3data(id3tag.get());
    }
}
//###################################################################
void Audio::latinToUTF8(ps_ptr<char>& buff, bool UTF8check) {
    // most stations send  strings in UTF-8 but a few sends in latin. To standardize this, all latin strings are
    // converted to UTF-8. If UTF-8 is already present, nothing is done and true is returned.
    // A conversion to UTF-8 extends the string. Therefore it is necessary to know the buffer size. If the converted
    // string does not fit into the buffer, false is returned

    uint16_t pos = 0;
    uint16_t in = 0;
    uint16_t out = 0;
    bool     isUTF8 = true;

    // We cannot detect if a given string (or byte sequence) is a UTF-8 encoded text as for example each and every series
    // of UTF-8 octets is also a valid (if nonsensical) series of Latin-1 (or some other encoding) octets.
    // However not every series of valid Latin-1 octets are valid UTF-8 series. So you can rule out strings that do not conform
    // to the UTF-8 encoding schema:

    if (UTF8check) {
        while (buff[pos] != '\0') {
            if (buff[pos] <= 0x7F) {// 0xxxxxxx: ASCII
                pos++;
            }
            else if ((buff[pos] & 0xE0) == 0xC0) {
                if (pos + 1 >= buff.strlen()) isUTF8 = false;
                // 110xxxxx 10xxxxxx: 2-byte
                if ((buff[pos + 1] & 0xC0) != 0x80) isUTF8 = false;
                if (buff[pos] < 0xC2) isUTF8 = false; // Overlong encoding
                pos += 2;
            }
            else if  ((buff[pos] & 0xF0) == 0xE0) {
                // 1110xxxx 10xxxxxx 10xxxxxx: 3-byte
                if ((buff[pos + 1] & 0xC0) != 0x80 || (buff[pos + 2] & 0xC0) != 0x80) isUTF8 = false;
                if (buff[pos] == 0xE0 && buff[pos + 1 ] < 0xA0) isUTF8 = false; // Overlong
                if (buff[pos] == 0xED && buff[pos + 1] >= 0xA0) isUTF8 = false; // UTF-16 surrogate
                pos += 3;
            }
            else if ((buff[pos] & 0xF8) == 0xF0) {
                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx: 4-byte
                if ((buff[pos + 1] & 0xC0) != 0x80 ||
                    (buff[pos + 2] & 0xC0) != 0x80 ||
                    (buff[pos + 3] & 0xC0) != 0x80) isUTF8 = false;
                if (buff[pos] == 0xF0 && buff[pos + 1] < 0x90) isUTF8 = false; // Overlong
                if (buff[pos] > 0xF4 || (buff[pos] == 0xF4 && buff[pos + 1] > 0x8F)) isUTF8 = false; // > U+10FFFF
                pos += 4;
            }
            else{
                isUTF8 = false; // Invalid first byte
            }
            if(!isUTF8)break;
        }
        if (isUTF8) return; // is UTF-8, do nothing
    }
    ps_ptr<char> iso8859_1;
    iso8859_1.assign(buff.get());

    // Worst-case: all chars are latin1 > 0x7F became 2 Bytes → max length is twice +1
    std::size_t requiredSize = strlen(iso8859_1.get()) * 2 + 1;

    if (buff.size() < requiredSize) {
        buff.realloc(requiredSize, "utf8_conv");
    }

    // coding into UTF-8
    while (iso8859_1[in] != '\0') {
        if (iso8859_1[in] < 0x80) {
            buff[out++] = iso8859_1[in++];
        } else {
            buff[out++] = (0xC0 | (iso8859_1[in] >> 6));
            buff[out++] = (0x80 | (iso8859_1[in] & 0x3F));
            in++;
        }
    }
    buff[out] = '\0';
}
//###################################################################
void Audio::htmlToUTF8(char* str) { // convert HTML to UTF-8

    typedef struct { // --- EntityMap Definition ---
        const char *name;
        uint32_t codepoint;
    } EntityMap;

    const EntityMap entities[] = {
        {"amp",   0x0026}, // &
        {"lt",    0x003C}, // <
        {"gt",    0x003E}, // >
        {"quot",  0x0022}, // "
        {"apos",  0x0027}, // '
        {"nbsp",  0x00A0}, // non-breaking space
        {"euro",  0x20AC}, // €
        {"copy",  0x00A9}, // ©
        {"reg",   0x00AE}, // ®
        {"trade", 0x2122}, // ™
        {"hellip",0x2026}, // …
        {"ndash", 0x2013}, // –
        {"mdash", 0x2014}, // —
        {"sect",  0x00A7}, // §
        {"para",  0x00B6}  // ¶
    };

    // --- EntityMap Lookup ---
    auto find_entity = [&](const char *p, uint32_t *codepoint, int *entity_len) {
        for (size_t i = 0; i < sizeof(entities)/sizeof(entities[0]); i++) {
            const char *name = entities[i].name;
            size_t len = strlen(name);
            if (strncmp(p + 1, name, len) == 0 && p[len + 1] == ';') {
                *codepoint = entities[i].codepoint;
                *entity_len = (int)(len + 2); // &name;
                return 1;
            }
        }
        return 0;
    };


    auto codepoint_to_utf8 = [&](uint32_t cp, char *dst) { // Convert a Codepoint (Unicode) to UTF-8, writes in DST, there is number of bytes back
        if (cp <= 0x7F) {
            dst[0] = cp;
            return 1;
        } else if (cp <= 0x7FF) {
            dst[0] = 0xC0 | (cp >> 6);
            dst[1] = 0x80 | (cp & 0x3F);
            return 2;
        } else if (cp <= 0xFFFF) {
            dst[0] = 0xE0 | (cp >> 12);
            dst[1] = 0x80 | ((cp >> 6) & 0x3F);
            dst[2] = 0x80 | (cp & 0x3F);
            return 3;
        } else if (cp <= 0x10FFFF) {
            dst[0] = 0xF0 | (cp >> 18);
            dst[1] = 0x80 | ((cp >> 12) & 0x3F);
            dst[2] = 0x80 | ((cp >> 6) & 0x3F);
            dst[3] = 0x80 | (cp & 0x3F);
            return 4;
        }
        return -1; // invalid Codepoint
    };

    char *p = str;
    while (*p != '\0') {
        if(p[0] == '&'){
            uint32_t cp;
            int consumed;
            if (find_entity(p, &cp, &consumed)) { // looking for entity, such as &copy;
                char utf8[5] = {0};
                int len = codepoint_to_utf8(cp, utf8);
                if (len > 0) {
                    size_t tail_len = strlen(p + consumed);
                    memmove(p + len, p + consumed, tail_len + 1);
                    memcpy(p, utf8, len);
                    p += len;
                    continue;
                }
            }
        }
        if (p[0] == '&' && p[1] == '#') {
            char *endptr;
            uint32_t codepoint = strtol(p + 2, &endptr, 10);

            if (*endptr == ';' && codepoint <= 0x10FFFF) {
                char utf8[5] = {0};
                int utf8_len = codepoint_to_utf8(codepoint, utf8);
                if (utf8_len > 0) {
                //    size_t entity_len = endptr - p + 1;
                    size_t tail_len = strlen(endptr + 1);

                    // Show residual ring to the left
                    memmove(p + utf8_len, endptr + 1, tail_len + 1);  // +1 because of '\0'

                    // Copy UTF-8 characters
                    memcpy(p, utf8, utf8_len);

                    // weiter bei neuem Zeichen
                    p += utf8_len;
                    continue;
                }
            }
        }
        p++;
    }
}

//###################################################################
size_t Audio::readAudioHeader(uint32_t bytes) {
    size_t bytesReaded = 0;
    if(m_codec == CODEC_WAV) {
        int res = read_WAV_Header(InBuff.getReadPtr(), bytes);
        if(res >= 0) bytesReaded = res;
        else { // error, skip header
            m_controlCounter = 100;
        }
    }
    if(m_codec == CODEC_MP3) {
        int res = read_ID3_Header(InBuff.getReadPtr(), bytes);
        if(res >= 0) bytesReaded = res;
        else { // error, skip header
            m_controlCounter = 100;
        }
    }
    if(m_codec == CODEC_M4A) {
        int res = read_M4A_Header(InBuff.getReadPtr(), bytes);
        if(res >= 0) bytesReaded = res;
        else { // error, skip header
            m_controlCounter = 100;
        }
    }
    if(m_codec == CODEC_AAC) {
        // stream only, no header
        m_audioDataSize = getFileSize();
        m_controlCounter = 100;
    }
    if(m_codec == CODEC_FLAC) {
        int res = read_FLAC_Header(InBuff.getReadPtr(), bytes);
        if(res >= 0) bytesReaded = res;
        else { // error, skip header
//            stopSong();
            m_controlCounter = 100;
        }
    }
    if(m_codec == CODEC_OPUS)   { m_controlCounter = 100; }
    if(m_codec == CODEC_VORBIS) { m_controlCounter = 100; }
    if(m_codec == CODEC_OGG)    { m_controlCounter = 100; }
    if(!isRunning()) {
        AUDIO_ERROR("Processing stopped due to invalid audio header");
        return 0;
    }
    return bytesReaded;
}
//##################################################################
int Audio::read_WAV_Header(uint8_t* data, size_t len) {

    if(m_controlCounter == 0) {
        m_rwh.cs = 0;
        m_rwh.bts = 0;
        m_controlCounter++;
        if((*data != 'R') || (*(data + 1) != 'I') || (*(data + 2) != 'F') || (*(data + 3) != 'F')) {
            AUDIO_INFO("file has no RIFF tag");
            m_rwh.headerSize = 0;
            return -1; // false;
        }
        else {
            m_rwh.headerSize = 4;
            return 4; // ok
        }
    }

    if(m_controlCounter == 1) {
        m_controlCounter++;
        m_rwh.cs = (uint32_t)(*data + (*(data + 1) << 8) + (*(data + 2) << 16) + (*(data + 3) << 24) - 8);
        m_rwh.headerSize += 4;
        return 4; // ok
    }

    if(m_controlCounter == 2) {
        m_controlCounter++;
        if((*data != 'W') || (*(data + 1) != 'A') || (*(data + 2) != 'V') || (*(data + 3) != 'E')) {
            AUDIO_INFO("format tag is not WAVE");
            return -1; // false;
        }
        else {
            m_rwh.headerSize += 4;
            return 4;
        }
    }

    if(m_controlCounter == 3) {
        if((*data == 'f') && (*(data + 1) == 'm') && (*(data + 2) == 't')) {
            m_controlCounter++;
            m_rwh.headerSize += 4;
            return 4;
        }
        else {
            m_rwh.headerSize += 4;
            return 4;
        }
    }

    if(m_controlCounter == 4) {
        m_controlCounter++;
        m_rwh.cs = (uint32_t)(*data + (*(data + 1) << 8));
        if(m_rwh.cs > 40) return -1; // false, something going wrong
        m_rwh.bts = m_rwh.cs - 16;         // bytes to skip if fmt chunk is >16
        m_rwh.headerSize += 4;
        return 4;
    }

    if(m_controlCounter == 5) {
        m_controlCounter++;
        uint16_t fc = (uint16_t)(*(data + 0) + (*(data + 1) << 8));                                               // Format code
        uint16_t nic = (uint16_t)(*(data + 2) + (*(data + 3) << 8));                                              // Number of interleaved channels
        uint32_t sr = (uint32_t)(*(data + 4) + (*(data + 5) << 8) + (*(data + 6) << 16) + (*(data + 7) << 24));   // Samplerate
        uint32_t dr = (uint32_t)(*(data + 8) + (*(data + 9) << 8) + (*(data + 10) << 16) + (*(data + 11) << 24)); // Datarate
        uint16_t dbs = (uint16_t)(*(data + 12) + (*(data + 13) << 8));                                            // Data block size
        uint16_t bps = (uint16_t)(*(data + 14) + (*(data + 15) << 8));                                            // Bits per sample

        AUDIO_INFO("FormatCode: %u", fc);
        // AUDIO_INFO("Channel: %u", nic);
        // AUDIO_INFO("SampleRate: %u", sr);
        AUDIO_INFO("DataRate: %lu", (long unsigned int)dr);
        AUDIO_INFO("DataBlockSize: %u", dbs);
        AUDIO_INFO("BitsPerSample: %u", bps);

        if((bps != 8) && (bps != 16)) {
            AUDIO_INFO("BitsPerSample is %u,  must be 8 or 16", bps);
            stopSong();
            return -1;
        }
        if((nic != 1) && (nic != 2)) {
            AUDIO_INFO("num channels is %u,  must be 1 or 2", nic);
            stopSong();
            return -1;
        }
        if(fc != 1) {
            AUDIO_INFO("format code is not 1 (PCM)");
            stopSong();
            return -1; // false;
        }
//        setBitsPerSample(bps);
//        setChannels(nic);
//        setSampleRate(sr);
        setBitrate(nic * sr * bps);
//        m_bitRate = nic * sr * bps;
        //    AUDIO_INFO("BitRate: %u", m_bitRate);
        m_rwh.headerSize += 16;
        return 16; // ok
    }

    if(m_controlCounter == 6) {
        m_controlCounter++;
        m_rwh.headerSize += m_rwh.bts;
        return m_rwh.bts; // skip to data
    }

    if(m_controlCounter == 7) {
        if((*(data + 0) == 'd') && (*(data + 1) == 'a') && (*(data + 2) == 't') && (*(data + 3) == 'a')) {
            m_controlCounter++;
            //    vTaskDelay(30);
            m_rwh.headerSize += 4;
            return 4;
        }
        else {
            m_rwh.headerSize++;
            return 1;
        }
    }

    if(m_controlCounter == 8) {
        m_controlCounter++;
        size_t cs = *(data + 0) + (*(data + 1) << 8) + (*(data + 2) << 16) + (*(data + 3) << 24); // read chunkSize
        m_rwh.headerSize += 4;
        if(m_dataMode == AUDIO_LOCALFILE) m_contentlength = getFileSize();
        if(cs) { m_audioDataSize = cs - 44; }
        else { // sometimes there is nothing here
            if(m_dataMode == AUDIO_LOCALFILE) m_audioDataSize = getFileSize() -m_rwh. headerSize;
            if(m_streamType == ST_WEBFILE) m_audioDataSize = m_contentlength - m_rwh.headerSize;
        }
        AUDIO_INFO("Audio-Length: %u", m_audioDataSize);
        return 4;
    }
    m_controlCounter = 100; // header succesfully read
    m_audioDataStart = m_rwh.headerSize;
    return 0;
}
//##################################################################
int Audio::read_FLAC_Header(uint8_t* data, size_t len) {

    if(m_rflh.retvalue) {
        if(m_rflh.retvalue > len) { // if returnvalue > bufferfillsize
            if(len > InBuff.getMaxBlockSize()) len = InBuff.getMaxBlockSize();
            m_rflh.retvalue -= len; // and wait for more bufferdata
            return len;
        }
        else {
            size_t tmp = m_rflh.retvalue;
            m_rflh.retvalue = 0;
            return tmp;
        }
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_BEGIN) { // init
        m_rflh.headerSize = 0;
        m_rflh.retvalue = 0;
        m_audioDataStart = 0;
        m_rflh.picPos = 0;
        m_rflh.picLen = 0;
        m_rflh.f_lastMetaBlock = false;
        m_controlCounter = FLAC_MAGIC;
        if(m_dataMode == AUDIO_LOCALFILE) {
            m_contentlength = getFileSize();
            AUDIO_INFO("Content-Length: %lu", (long unsigned int)m_contentlength);
        }
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_MAGIC) {            /* check MAGIC STRING */
        if(specialIndexOf(data, "OggS", 10) == 0) { // is ogg
            m_rflh.headerSize = 0;
            m_rflh.retvalue = 0;
            m_controlCounter = FLAC_OKAY;
            return 0;
        }
        if(specialIndexOf(data, "fLaC", 10) != 0) {
            AUDIO_ERROR("Magic String 'fLaC' not found in header");
            stopSong();
            return -1;
        }
        m_controlCounter = FLAC_MBH; // METADATA_BLOCK_HEADER
        m_rflh.headerSize = 4;
        m_rflh.retvalue = 4;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_MBH) { /* METADATA_BLOCK_HEADER */
        uint8_t blockType = *data;
        if(!m_rflh.f_lastMetaBlock) {
            if(blockType & 128) {m_rflh.f_lastMetaBlock = true; }
            blockType &= 127;
            if(blockType == 0) m_controlCounter = FLAC_SINFO;
            if(blockType == 1) m_controlCounter = FLAC_PADDING;
            if(blockType == 2) m_controlCounter = FLAC_APP;
            if(blockType == 3) m_controlCounter = FLAC_SEEK;
            if(blockType == 4) m_controlCounter = FLAC_VORBIS;
            if(blockType == 5) m_controlCounter = FLAC_CUESHEET;
            if(blockType == 6) m_controlCounter = FLAC_PICTURE;
            m_rflh.headerSize += 1;
            m_rflh.retvalue = 1;
            return 0;
        }
        m_controlCounter = FLAC_OKAY;
        m_audioDataStart =m_rflh. headerSize;
        m_audioDataSize = m_contentlength - m_audioDataStart;
//        FLACSetRawBlockParams(m_flacNumChannels, m_flacSampleRate, m_flacBitsPerSample, m_flacTotalSamplesInStream, m_audioDataSize);
        if(m_rflh.picLen) {
            size_t pos = m_audiofile.position();
            if(audio_id3image) audio_id3image(m_audiofile, m_rflh.picPos, m_rflh.picLen);
            m_audiofile.seek(pos); // the filepointer could have been changed by the user, set it back
        }
        AUDIO_INFO("Audio-Length: %u", m_audioDataSize);
        if(audio_progress) audio_progress(m_audioDataStart, m_audioDataSize);
        m_rflh.retvalue = 0;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_SINFO) { /* Stream info block */
        size_t l = bigEndian(data, 3);
        vTaskDelay(2);
        m_flacMaxBlockSize = bigEndian(data + 5, 2);
        AUDIO_INFO("FLAC maxBlockSize: %u", m_flacMaxBlockSize);
        vTaskDelay(2);
        m_flacMaxFrameSize = bigEndian(data + 10, 3);
        if(m_flacMaxFrameSize) { AUDIO_INFO("FLAC maxFrameSize: %u", m_flacMaxFrameSize); }
        else { AUDIO_INFO("FLAC maxFrameSize: N/A"); }
        if(m_flacMaxFrameSize > InBuff.getMaxBlockSize()) {
//            AUDIO_ERROR("FLAC maxFrameSize too large!");
            AUDIO_INFO("FLAC maxFrameSize too large! changeMaxBlockSize to %i", m_flacMaxFrameSize);
            InBuff.changeMaxBlockSize(m_flacMaxFrameSize);
//            stopSong();
//            return -1;
        }
//             InBuff.changeMaxBlockSize(m_flacMaxFrameSize);
        vTaskDelay(2);
        uint32_t nextval = bigEndian(data + 13, 3);
        m_flacSampleRate = nextval >> 4;
        AUDIO_INFO("FLAC sampleRate: %lu", (long unsigned int)m_flacSampleRate);
        vTaskDelay(2);
        m_flacNumChannels = ((nextval & 0x06) >> 1) + 1;
        AUDIO_INFO("FLAC numChannels: %u", m_flacNumChannels);
        vTaskDelay(2);
        uint8_t bps = (nextval & 0x01) << 4;
        bps += (*(data + 16) >> 4) + 1;
        m_flacBitsPerSample = bps;
        if((bps != 8) && (bps != 16) && (bps != 24)) {
            AUDIO_ERROR("bits per sample must be 8, 16 or 24, is %i", bps);
//            stopSong();
//            return -1;
        }
        AUDIO_INFO("FLAC bitsPerSample: %u", m_flacBitsPerSample);
        m_flacTotalSamplesInStream = bigEndian(data + 17, 4);
        if(m_flacTotalSamplesInStream) { AUDIO_INFO("total samples in stream: %lu", (long unsigned int)m_flacTotalSamplesInStream); }
        else { AUDIO_INFO("total samples in stream: N/A"); }
        if(bps != 0 && m_flacTotalSamplesInStream) { AUDIO_INFO("audio file duration: %lu seconds", (long unsigned int)m_flacTotalSamplesInStream / (long unsigned int)m_flacSampleRate); }
        m_controlCounter = FLAC_MBH; // METADATA_BLOCK_HEADER
        m_rflh.retvalue = l + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_PADDING) { /* PADDING */
        size_t l = bigEndian(data, 3);
        m_controlCounter = FLAC_MBH;
        m_rflh.retvalue = l + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_APP) { /* APPLICATION */
        size_t l = bigEndian(data, 3);
        m_controlCounter = FLAC_MBH;
        m_rflh.retvalue = l + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_SEEK) { /* SEEKTABLE */
        size_t l = bigEndian(data, 3);
        m_controlCounter = FLAC_MBH;
        m_rflh.retvalue = l + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_VORBIS) { /* VORBIS COMMENT */ // field names
        ps_ptr<char>vendorString = {};
        size_t vendorLength = bigEndian(data, 3);
        size_t idx = 0;
        data += 3; idx += 3;
        size_t vendorStringLength = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
        if(vendorStringLength) {data += 4; idx += 4;}
        if(vendorStringLength > 495) vendorStringLength = 495; // guard
        vendorString.assign((const char*)data, vendorStringLength, "vendorString");
        vendorString.insert("VENDOR_STRING: ", 0);
        if(audio_id3data) audio_id3data(vendorString.get());
        data += vendorStringLength; idx += vendorStringLength;
        size_t commentListLength = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
        data += 4; idx += 4;

        char* album = NULL;
        char* artist = NULL;
        char* title  = NULL;

        for(int i = 0; i < commentListLength; i++) {
            (void)i;
            size_t commentLength = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
            data += 4; idx += 4;
            if(commentLength) { // guard
                vendorString.assign((const char*)data , commentLength, "vendorString");
                if(audio_id3data) audio_id3data(vendorString.get());

            if(startsWith(vendorString.get(), "album") || startsWith(vendorString.get(), "ALBUM")){
                album = strdup(vendorString.get() + 6);
                if(audio_id3album) audio_id3album(album); }

            if(startsWith(vendorString.get(), "artist") || startsWith(vendorString.get(), "ARTIST")){
                artist = strdup(vendorString.get() + 7);
                if(audio_id3artist) audio_id3artist(artist); }

            if(startsWith(vendorString.get(), "title") || startsWith(vendorString.get(), "TITLE")){
                title = strdup(vendorString.get() + 6);
                strcpy(album, " - ");
                strcpy(album, title);
//                if(audio_id3album) audio_id3album(album); 
                if(audio_id3title) audio_id3title(album); }

            }
            data += commentLength; idx += commentLength;
            if(idx > vendorLength + 3) {AUDIO_ERROR("VORBIS COMMENT section is too long");}
        }
        m_controlCounter = FLAC_MBH;
        m_rflh.retvalue = vendorLength + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_CUESHEET) { /* CUESHEET */
        size_t l = bigEndian(data, 3);
        m_controlCounter = FLAC_MBH;
        m_rflh.retvalue = l + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == FLAC_PICTURE) { /* PICTURE */
        m_rflh.picLen = bigEndian(data, 3);
        m_rflh.picPos = m_rflh.headerSize;
        // log_w("FLAC PICTURE, size %i, pos %i", picLen, picPos);
        m_controlCounter = FLAC_MBH;
        m_rflh.retvalue = m_rflh.picLen + 3;
        m_rflh.headerSize += m_rflh.retvalue;
        return 0;
    }
    return 0;
}
//##################################################################
int Audio::read_ID3_Header(uint8_t* data, size_t len) {

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 0) { /* read ID3 tag and ID3 header size */
        m_controlCounter++;

        m_ID3Hdr.id3Size = 0;
        m_ID3Hdr.totalId3Size = 0; // if we have more header, id3_1_size + id3_2_size + ....
        m_ID3Hdr.remainingHeaderBytes = 0;
        m_ID3Hdr.universal_tmp = 0;
        m_ID3Hdr.ID3version = 0;
        m_ID3Hdr.ehsz = 0;
        m_ID3Hdr.framesize = 0;
        m_ID3Hdr.compressed = false;
        m_ID3Hdr.SYLT_seen = false;
        m_ID3Hdr.SYLT_size = 0;
        m_ID3Hdr.SYLT_pos = 0;
        m_ID3Hdr.numID3Header = 0;
        m_ID3Hdr.iBuffSize = 4096;
        m_ID3Hdr.iBuff.alloc(m_ID3Hdr.iBuffSize + 10, "iBuff");
        memset(m_ID3Hdr.tag, 0, sizeof(m_ID3Hdr.tag));
        memset(m_ID3Hdr.APIC_size, 0, sizeof(m_ID3Hdr.APIC_size));
        memset(m_ID3Hdr.APIC_pos, 0, sizeof(m_ID3Hdr.APIC_pos));
        memset(m_ID3Hdr.tag, 0, sizeof(m_ID3Hdr.tag));

        if(m_dataMode == AUDIO_LOCALFILE) {
            m_ID3Hdr.ID3version = 0;
            m_contentlength = getFileSize();
            AUDIO_INFO("Content-Length: %lu", (long unsigned int)m_contentlength);
        }

        m_ID3Hdr.SYLT_seen = false;
        m_ID3Hdr.remainingHeaderBytes = 0;
        m_ID3Hdr.ehsz = 0;
        if(specialIndexOf(data, "ID3", 4) != 0) { // ID3 not found
            if(!m_f_m3u8data) {AUDIO_INFO("file has no ID3 tag, skip metadata");}
            m_audioDataSize = m_contentlength;
            if(!m_f_m3u8data) AUDIO_INFO("Audio-Length: %u", m_audioDataSize);
            if(audio_progress) audio_progress(m_audioDataStart, m_audioDataSize);
            return -1; // error, no ID3 signature found
        }
        m_ID3Hdr.ID3version = *(data + 3);
        switch(m_ID3Hdr.ID3version) {
            case 2:
                m_f_unsync = (*(data + 5) & 0x80);
                m_f_exthdr = false;
                break;
            case 3:
            case 4:
                m_f_unsync = (*(data + 5) & 0x80); // bit7
                m_f_exthdr = (*(data + 5) & 0x40); // bit6 extended header
                break;
        };
        m_ID3Hdr.id3Size = bigEndian(data + 6, 4, 7); //  ID3v2 size  4 * %0xxxxxxx (shift left seven times!!)
        m_ID3Hdr.id3Size += 10;

        // Every read from now may be unsync'd
        if(!m_f_m3u8data) AUDIO_INFO("ID3 framesSize: %i", m_ID3Hdr.id3Size);
        if(!m_f_m3u8data) AUDIO_INFO("ID3 version: 2.%i", m_ID3Hdr.ID3version);

        if(m_ID3Hdr.ID3version == 2) { m_controlCounter = 10; }
        m_ID3Hdr.remainingHeaderBytes = m_ID3Hdr.id3Size;
        m_ID3Size = m_ID3Hdr.id3Size;
        m_ID3Hdr.remainingHeaderBytes -= 10;

        return 10;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 1) { // compute extended header size if exists
        m_controlCounter++;
        if(m_f_exthdr) {
            AUDIO_INFO("ID3 extended header");
            m_ID3Hdr.ehsz = bigEndian(data, 4);
            m_ID3Hdr.remainingHeaderBytes -= 4;
            m_ID3Hdr.ehsz -= 4;
            return 4;
        }
        else {
            if(!m_f_m3u8data) AUDIO_INFO("ID3 normal frames");
            return 0;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 2) { // skip extended header if exists
        if(m_ID3Hdr.ehsz > len) {
            m_ID3Hdr.ehsz -= len;
            m_ID3Hdr.remainingHeaderBytes -= len;
            return len;
        } // Throw it away
        else {
            m_controlCounter++;
            m_ID3Hdr.remainingHeaderBytes -= m_ID3Hdr.ehsz;
            return m_ID3Hdr.ehsz;
        } // Throw it away
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 3) { // read a ID3 frame, get the tag
        if(m_ID3Hdr.remainingHeaderBytes == 0) {
            m_controlCounter = 99;
            return 0;
        }
        m_controlCounter++;
        m_ID3Hdr.frameid[0] = *(data + 0);
        m_ID3Hdr.frameid[1] = *(data + 1);
        m_ID3Hdr.frameid[2] = *(data + 2);
        m_ID3Hdr.frameid[3] = *(data + 3);
        m_ID3Hdr.frameid[4] = 0;
        for(uint8_t i = 0; i < 4; i++) m_ID3Hdr.tag[i] = m_ID3Hdr.frameid[i]; // tag = frameid

        m_ID3Hdr.remainingHeaderBytes -= 4;
        if(m_ID3Hdr.frameid[0] == 0 && m_ID3Hdr.frameid[1] == 0 && m_ID3Hdr.frameid[2] == 0 && m_ID3Hdr.frameid[3] == 0) {
            // We're in padding
            m_controlCounter = 98; // all ID3 metadata processed
        }
        return 4;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 4) { // get the frame size
        m_controlCounter = 6;

        if(m_ID3Hdr.ID3version == 4) {
            m_ID3Hdr.framesize = bigEndian(data, 4, 7); // << 7
        }
        else {
            m_ID3Hdr.framesize = bigEndian(data, 4); // << 8
        }
        m_ID3Hdr.remainingHeaderBytes -= 4;
        uint8_t frameFlag_0 = *(data + 4);
        uint8_t frameFlag_1 = *(data + 5);
        (void)frameFlag_0;
        m_ID3Hdr.remainingHeaderBytes--;
        m_ID3Hdr.compressed = frameFlag_1 & 0x80; // Frame is compressed using [#ZLIB zlib] with 4 bytes for 'decompressed
        m_ID3Hdr.remainingHeaderBytes--;
        uint32_t decompsize = 0;
        if(m_ID3Hdr.compressed) {
            // log_i("iscompressed");
            decompsize = bigEndian(data + 6, 4);
            m_ID3Hdr.remainingHeaderBytes -= 4;
            (void)decompsize;
            // log_i("decompsize=%u", decompsize);
            return 6 + 4;
        }
        return 6;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 5) { // If the frame is larger than m_ID3Hdr.framesize, skip the rest
        if(m_ID3Hdr.framesize > len) {
            m_ID3Hdr.framesize -= len;
            m_ID3Hdr.remainingHeaderBytes -= len;
            return len;
        }
        else {
            m_controlCounter = 3; // check next frame
            m_ID3Hdr.remainingHeaderBytes -= m_ID3Hdr.framesize;
            return m_ID3Hdr.framesize;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 6) { // Read the value
        m_controlCounter = 5;   // only read 256 bytes
        uint8_t textEncodingByte = *(data + 0);  // ID3v2 Text-Encoding-Byte
        // $00 – ISO-8859-1 (LATIN-1, Identical to ASCII for values smaller than 0x80).
        // $01 – UCS-2 encoded Unicode with BOM (Byte Order Mark), in ID3v2.2 and ID3v2.3.
        // $02 – UTF-16BE encoded Unicode without BOM (Byte Order Mark) , in ID3v2.4.
        // $03 – UTF-8 encoded Unicode, in ID3v2.4.

        if(startsWith(m_ID3Hdr.tag, "APIC")) { // a image embedded in file, passing it to external function
        //    if(m_dataMode == AUDIO_LOCALFILE) {
                m_ID3Hdr.APIC_pos[m_ID3Hdr.numID3Header] = m_ID3Hdr.totalId3Size + m_ID3Hdr.id3Size - m_ID3Hdr.remainingHeaderBytes;
                m_ID3Hdr.APIC_size[m_ID3Hdr.numID3Header] = m_ID3Hdr.framesize;
                //    AUDIO_ERROR("APIC_pos %i APIC_size %i", APIC_pos[numID3Header], APIC_size[numID3Header]);
        //    }
            return 0;
        }

        if( // any lyrics embedded in file, passing it to external function
            startsWith(m_ID3Hdr.tag, "SYLT") || startsWith(m_ID3Hdr.tag, "TXXX") || startsWith(m_ID3Hdr.tag, "USLT")) {
            if(m_dataMode == AUDIO_LOCALFILE) {
                m_ID3Hdr.SYLT_seen = true;
                m_ID3Hdr.SYLT_pos = m_ID3Hdr.id3Size - m_ID3Hdr.remainingHeaderBytes;
                m_ID3Hdr.SYLT_size = m_ID3Hdr.framesize;
            }
            return 0;
        }

        if( // proprietary not standard information
            startsWith(m_ID3Hdr.tag, "PRIV")) {
                ;//AUDIO_ERROR("PRIV");
                return 0;
        }



        if(m_ID3Hdr.framesize == 0) return 0;

        size_t fs = m_ID3Hdr.framesize; // fs = size of the frame data field as read from header
        size_t bytesToCopy = fs;

        if (bytesToCopy >= m_ID3Hdr.iBuffSize) { // <= oder >= hier ist wichtig!
            bytesToCopy = m_ID3Hdr.iBuffSize - 1; // Sicherstellen, dass ein Null-Terminator passt
        }
        size_t textDataLength = 0;
        if (bytesToCopy > 0) { // Nur wenn überhaupt Daten da sind, die wir kürzen können
            textDataLength = bytesToCopy - 1; // Dies ist die Anzahl der zu kopierenden TEXT-Bytes
        }
        for(int i = 0; i < textDataLength; i++) {
            m_ID3Hdr.iBuff[i] = *(data + i + 1); // Überspringt das erste Byte (Encoding)
        }
        m_ID3Hdr.iBuff[textDataLength] = 0;
        m_ID3Hdr.framesize -= fs;
        m_ID3Hdr.remainingHeaderBytes -= fs;
        uint16_t dataLength = fs - 1;

        if(textEncodingByte == 0){  // latin
            latinToUTF8(m_ID3Hdr.iBuff, false);
            showID3Tag(m_ID3Hdr.tag, m_ID3Hdr.iBuff.get());
        }

        if(textEncodingByte == 1  && dataLength > 1) { // UTF16 with BOM
            int8_t data_start = 0;
            if(startsWith(m_ID3Hdr.tag, "COMM")){ // language code
                m_ID3Hdr.lang[0] = m_ID3Hdr.iBuff[0];
                m_ID3Hdr.lang[1] = m_ID3Hdr.iBuff[1];
                m_ID3Hdr.lang[2] = m_ID3Hdr.iBuff[2];
                m_ID3Hdr.lang[3] = '\0';
                data_start += 3;
                // log_w("language code: %s", m_ID3Hdr.lang);
                m_ID3Hdr.byteOrderMark = static_cast<unsigned char>(m_ID3Hdr.iBuff[data_start]) == 0xFE && static_cast<unsigned char>(m_ID3Hdr.iBuff[data_start]) == 0xFF;
                data_start += 2;
                m_ID3Hdr.contentDescriptorTerminator_0 = m_ID3Hdr.iBuff[data_start];
                m_ID3Hdr.contentDescriptorTerminator_1 = m_ID3Hdr.iBuff[data_start + 1];
                m_ID3Hdr.textStringTerminator_0        = m_ID3Hdr.iBuff[data_start + 2];
                m_ID3Hdr.textStringTerminator_1        = m_ID3Hdr.iBuff[data_start + 3];
                data_start += 4;
            }
            else{
                m_ID3Hdr.byteOrderMark = static_cast<unsigned char>(m_ID3Hdr.iBuff[data_start]) == 0xFE && static_cast<unsigned char>(m_ID3Hdr.iBuff[data_start]) == 0xFF;
                data_start += 2;
            }

            std::u16string utf16_string;
            for (size_t i = data_start; i < dataLength; i += 2) {
                char16_t wchar;
                if(m_ID3Hdr.byteOrderMark)  wchar = (static_cast<unsigned char>(m_ID3Hdr.iBuff[i]) << 8) | static_cast<unsigned char>(m_ID3Hdr.iBuff[i + 1]);
                else                      wchar = (static_cast<unsigned char>(m_ID3Hdr.iBuff[i + 1]) << 8) | static_cast<unsigned char>(m_ID3Hdr.iBuff[i]);
                utf16_string.push_back(wchar);
            }

            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
            showID3Tag(m_ID3Hdr.tag, converter.to_bytes(utf16_string).c_str());
        }

        if(textEncodingByte == 2 && dataLength > 1) { // UTF16BE

            std::u16string utf16_string;
            for (size_t i = 0; i < dataLength; i += 2) {
                char16_t  wchar = (static_cast<unsigned char>(m_ID3Hdr.iBuff[i]) << 8) | static_cast<unsigned char>(m_ID3Hdr.iBuff[i + 1]);
                utf16_string.push_back(wchar);
            }

            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
            showID3Tag(m_ID3Hdr.tag, converter.to_bytes(utf16_string).c_str());
        }

        if(textEncodingByte == 3) { // utf8
            showID3Tag(m_ID3Hdr.tag, m_ID3Hdr.iBuff.get());
        }
        return fs;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // --- section V2.2 only , greater Vers above ----
    // see https://mutagen-specs.readthedocs.io/en/latest/id3/id3v2.2.html
    if(m_controlCounter == 10) { // frames in V2.2, 3bytes identifier, 3bytes size descriptor

        if(m_ID3Hdr.universal_tmp > 0) {
            if(m_ID3Hdr.universal_tmp > len) {
                m_ID3Hdr.universal_tmp -= len;
                return len;
            } // Throw it away
            else {
                uint32_t t = m_ID3Hdr.universal_tmp;
                m_ID3Hdr.universal_tmp = 0;
                return t;
            } // Throw it away
        }

        m_ID3Hdr.frameid[0] = *(data + 0);
        m_ID3Hdr.frameid[1] = *(data + 1);
        m_ID3Hdr.frameid[2] = *(data + 2);
        m_ID3Hdr.frameid[3] = 0;
        for(uint8_t i = 0; i < 4; i++) m_ID3Hdr.tag[i] = m_ID3Hdr.frameid[i]; // tag = frameid
        m_ID3Hdr.remainingHeaderBytes -= 3;
        size_t dataLen = bigEndian(data + 3, 3);
        m_ID3Hdr.universal_tmp = dataLen;
        m_ID3Hdr.remainingHeaderBytes -= 3;
        char value[256];
        if(dataLen > 249) { dataLen = 249; }
        memcpy(value, (data + 7), dataLen);
        value[dataLen + 1] = 0;
        if(startsWith(m_ID3Hdr.tag, "PIC")) { // image embedded in header
            if(m_dataMode == AUDIO_LOCALFILE) {
                m_ID3Hdr.APIC_pos[m_ID3Hdr.numID3Header] = m_ID3Hdr.id3Size - m_ID3Hdr.remainingHeaderBytes;
                m_ID3Hdr.APIC_size[m_ID3Hdr.numID3Header] = m_ID3Hdr.universal_tmp;
                // log_i("Attached picture seen at pos %d length %d", APIC_pos[0], APIC_size[0]);
            }
        }
        else if(startsWith(m_ID3Hdr.tag, "SLT")) { // lyrics embedded in header
            if(m_dataMode == AUDIO_LOCALFILE) {
                m_ID3Hdr.SYLT_seen = true; // #460
                m_ID3Hdr.SYLT_pos = m_ID3Hdr.id3Size - m_ID3Hdr.remainingHeaderBytes;
                m_ID3Hdr.SYLT_size = m_ID3Hdr.universal_tmp;
                // log_i("Attached lyrics seen at pos %d length %d", SYLT_pos, SYLT_size);
            }
        }
        else { showID3Tag(m_ID3Hdr.tag, value);}
        m_ID3Hdr.remainingHeaderBytes -= m_ID3Hdr.universal_tmp;
        m_ID3Hdr.universal_tmp -= dataLen;

        if(dataLen == 0) m_controlCounter = 98;
        if(m_ID3Hdr.remainingHeaderBytes == 0) m_controlCounter = 98;

        return 3 + 3 + dataLen;
    }
    // -- end section V2.2 -----------

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 98) { // skip all ID3 metadata (mostly spaces)
        if(m_ID3Hdr.remainingHeaderBytes > len) {
            m_ID3Hdr.remainingHeaderBytes -= len;
            return len;
        } // Throw it away
        else {
            m_controlCounter = 99;
            return m_ID3Hdr.remainingHeaderBytes;
        } // Throw it away
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == 99) { //  exist another ID3tag?
        m_audioDataStart += m_ID3Hdr.id3Size;
        //    vTaskDelay(30);
        if((*(data + 0) == 'I') && (*(data + 1) == 'D') && (*(data + 2) == '3')) {
            m_controlCounter = 0;
            m_ID3Hdr.numID3Header++;
            m_ID3Hdr.totalId3Size += m_ID3Hdr.id3Size;
            return 0;
        }
        else {
            m_controlCounter = 100; // ok
            m_audioDataSize = m_contentlength - m_audioDataStart;
            if(!m_f_m3u8data) AUDIO_INFO("Audio-Length: %u", m_audioDataSize);
            if(audio_progress) audio_progress(m_audioDataStart, m_audioDataSize);
            if(m_ID3Hdr.APIC_pos[0] && audio_id3image) { // if we have more than one APIC, output the first only
                size_t pos = m_audiofile.position();
                audio_id3image(m_audiofile, m_ID3Hdr.APIC_pos[0], m_ID3Hdr.APIC_size[0]);
                m_audiofile.seek(pos); // the filepointer could have been changed by the user, set it back
            }
            if(m_ID3Hdr.SYLT_seen && audio_id3lyrics) {
                size_t pos = m_audiofile.position();
                audio_id3lyrics(m_audiofile, m_ID3Hdr.SYLT_pos, m_ID3Hdr.SYLT_size);
                m_audiofile.seek(pos); // the filepointer could have been changed by the user, set it back
            }
            m_ID3Hdr.numID3Header = 0;
            m_ID3Hdr.totalId3Size = 0;
            for(int i = 0; i < 3; i++) m_ID3Hdr.APIC_pos[i] = 0;  // delete all
            for(int i = 0; i < 3; i++) m_ID3Hdr.APIC_size[i] = 0; // delete all
            m_ID3Hdr.iBuff.reset();
            return 0;
        }
    }
    return 0;
}
//###################################################################
int Audio::read_M4A_Header(uint8_t* data, size_t len) {

    // Lambda function for Variant length determination
    auto parse_variant_length = [](uint8_t *&ptr) -> int {
        int length = 0;
        do {
            length = (length << 7) | (*ptr & 0x7F);  // Read the lower 7 bits of the current byte
        } while (*(ptr++) & 0x80);  //Increment the pointer after each byte
        return length;
    };

    /*
         ftyp
           | - moov  -> trak -> ... -> mp4a contains raw block parameters
           |    L... -> ilst  contains artist, composer ....
         free (optional) // jump to another atoms at the end of mdat
           |
         mdat contains the audio data                                                      */

    if(m_controlCounter == M4A_BEGIN) m_m4aHdr.retvalue = 0;

    if(m_m4aHdr.retvalue) {
        if(len > InBuff.getMaxBlockSize()) len = InBuff.getMaxBlockSize();
        if(m_m4aHdr.retvalue > len) { // if returnvalue > bufferfillsize
            m_m4aHdr.retvalue -= len; // and wait for more bufferdata
            m_m4aHdr.cnt += len;
            return len;
        }
        else {
            size_t tmp = m_m4aHdr.retvalue;
            m_m4aHdr.retvalue = 0;
            m_m4aHdr.cnt += tmp;
            m_m4aHdr.cnt = 0;
            return tmp;
        }
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == M4A_BEGIN) { // init
        m_m4aHdr.headerSize = 0;
        m_m4aHdr.retvalue = 0;
        m_m4aHdr.atomsize = 0;
        m_m4aHdr.audioDataPos = 0;
        m_m4aHdr.cnt = 0;
        m_m4aHdr.picPos = 0;
        m_m4aHdr.picLen = 0;
        m_controlCounter = M4A_FTYP;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == M4A_FTYP) { /* check_m4a_file */
        m_m4aHdr.atomsize = bigEndian(data, 4); // length of first atom
        if(specialIndexOf(data, "ftyp", 10) != 4) {
            AUDIO_ERROR("atom 'ftyp' not found in header");
            stopSong();
            return -1;
        }
        int m4a = specialIndexOf(data, "M4A ", 20);
        int isom = specialIndexOf(data, "isom", 20);
        int mp42 = specialIndexOf(data, "mp42", 20);

        if((m4a != 8) && (isom != 8) && (mp42 != 8)) {
            AUDIO_ERROR("subtype 'MA4 ', 'isom' or 'mp42' expected, but found '%s '", (data + 8));
//            stopSong();
//            return -1;
        }

        m_controlCounter = M4A_CHK;
        m_m4aHdr.retvalue = m_m4aHdr.atomsize;
        m_m4aHdr.headerSize = m_m4aHdr.atomsize;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == M4A_CHK) {  /* check  Tag */
        m_m4aHdr.atomsize = bigEndian(data, 4); // length of this atom
        if(specialIndexOf(data, "moov", 10) == 4) {
            m_controlCounter = M4A_MOOV;
            return 0;
        }
        else if(specialIndexOf(data, "free", 10) == 4) {
            m_m4aHdr.retvalue = m_m4aHdr.atomsize;
            m_m4aHdr.headerSize += m_m4aHdr.atomsize;
            return 0;
        }
        else if(specialIndexOf(data, "mdat", 10) == 4) {
            m_controlCounter = M4A_MDAT;
            return 0;
        }
        else {
            char atomName[5] = {0};
            (void)atomName;
            atomName[0] = *data;
            atomName[1] = *(data + 1);
            atomName[2] = *(data + 2);
            atomName[3] = *(data + 3);
            atomName[4] = 0;

            // log_i("atom %s found", atomName);

            m_m4aHdr.retvalue = m_m4aHdr.atomsize;
            m_m4aHdr.headerSize += m_m4aHdr.atomsize;
            return 0;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == M4A_MOOV) { // moov
        // we are looking for track and ilst
        if(specialIndexOf(data, "trak", len) > 0) {
            int offset = specialIndexOf(data, "trak", len);
            m_m4aHdr.retvalue = offset;
            m_m4aHdr.atomsize -= offset;
            m_m4aHdr.headerSize += offset;
            m_controlCounter = M4A_TRAK;
            return 0;
        }
        if(specialIndexOf(data, "ilst", len) > 0) {
            int offset = specialIndexOf(data, "ilst", len);
            m_m4aHdr.retvalue = offset;
            m_m4aHdr.atomsize -= offset;
            m_m4aHdr.headerSize += offset;
            m_controlCounter = M4A_ILST;
            return 0;
        }
        m_controlCounter = M4A_CHK;
        m_m4aHdr.headerSize += m_m4aHdr.atomsize;
        m_m4aHdr.retvalue = m_m4aHdr.atomsize;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == M4A_TRAK) { // trak
        if(specialIndexOf(data, "esds", len) > 0) {
            int      esds = specialIndexOf(data, "esds", len); // Packaging/Encapsulation And Setup Data
            uint8_t* pos = data + esds;
            pos += 8; // skip header

            if(*pos == 0x03) {;} // Found ES Descriptor (Tag: 0x03)
            pos++;
            int es_descriptor_len = parse_variant_length(pos); (void)es_descriptor_len;
            uint16_t es_id = (pos[0] << 8) | pos[1]; (void)es_id;
            uint8_t flags = pos[2]; (void)flags;
            pos += 3; // skip ES Descriptor data

            if (*pos == 0x04) {;}
            pos++; // skip tag

            int decoder_config_len = parse_variant_length(pos); (void)decoder_config_len;
            uint8_t object_type_indication = pos[0];

            if     (object_type_indication == (uint8_t)0x40) { AUDIO_INFO("AudioType: MPEG4 / Audio"); } // ObjectTypeIndication
            else if(object_type_indication == (uint8_t)0x66) { AUDIO_INFO("AudioType: MPEG2 / Audio"); }
            else if(object_type_indication == (uint8_t)0x69) { AUDIO_INFO("AudioType: MPEG2 / Audio Part 3"); } // Backward Compatible Audio
            else if(object_type_indication == (uint8_t)0x6B) { AUDIO_INFO("AudioType: MPEG1 / Audio"); }
            else { AUDIO_INFO("unknown Audio Type %x", object_type_indication); }

            pos++;
            uint8_t streamType = *pos >> 2;   // The upper 6 Bits are the StreamType
            if(streamType != 0x05) { AUDIO_ERROR("Streamtype is not audio!"); }
            pos += 4; // ST + BufferSizeDB.
            uint32_t maxBr = bigEndian(pos, 4); // max bitrate
            pos += 4;
            AUDIO_INFO("max bitrate: %lu", (long unsigned int)maxBr);

            uint32_t avrBr = bigEndian(pos, 4); // avg bitrate
            pos += 4;
            AUDIO_INFO("avg bitrate: %lu", (long unsigned int)avrBr);

            if ( *pos == 0x05) {;} // log_w("Found  DecoderSpecificInfo Tag (Tag: 0x05)")
            pos++;
            int decoder_specific_len = parse_variant_length((pos)); (void)decoder_specific_len;

            uint16_t ASC = bigEndian(pos, 2);

            uint8_t objectType = ASC >> 11; // first 5 bits

            if     (objectType == 1) { AUDIO_INFO("AudioObjectType: AAC Main"); } // Audio Object Types
            else if(objectType == 2) { AUDIO_INFO("AudioObjectType: AAC Low Complexity"); }
            else if(objectType == 3) { AUDIO_INFO("AudioObjectType: AAC Scalable Sample Rate"); }
            else if(objectType == 4) { AUDIO_INFO("AudioObjectType: AAC Long Term Prediction"); }
            else if(objectType == 5) { AUDIO_INFO("AudioObjectType: AAC Spectral Band Replication"); }
            else if(objectType == 6) { AUDIO_INFO("AudioObjectType: AAC Scalable"); }
            else { AUDIO_INFO("unknown ObjectType %x, stop", objectType); stopSong();}
            if(objectType < 7) m_M4A_objectType = objectType;

            const uint32_t samplingFrequencies[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
            uint8_t        sRate = (ASC & 0x0600) >> 7; // next 4 bits Sampling Frequencies
            AUDIO_INFO("Sampling Frequency: %lu", (long unsigned int)samplingFrequencies[sRate]);

            uint8_t chConfig = (ASC & 0x78) >> 3; // next 4 bits
            if(chConfig == 0) AUDIO_INFO("Channel Configurations: AOT Specifc Config");
            if(chConfig == 1) AUDIO_INFO("Channel Configurations: front-center");
            if(chConfig == 2) AUDIO_INFO("Channel Configurations: front-left, front-right");
            if(chConfig > 2) { AUDIO_ERROR("Channel Configurations with more than 2 channels is not allowed, stop!"); stopSong();}
            if(chConfig < 3) m_M4A_chConfig = chConfig;

            uint8_t frameLengthFlag = (ASC & 0x04);
            uint8_t dependsOnCoreCoder = (ASC & 0x02);
            (void)dependsOnCoreCoder;
            uint8_t extensionFlag = (ASC & 0x01);
            (void)extensionFlag;

            if(frameLengthFlag == 0) AUDIO_INFO("AAC FrameLength: 1024 bytes");
            if(frameLengthFlag == 1) AUDIO_INFO("AAC FrameLength: 960 bytes");
        }
        if(specialIndexOf(data, "mp4a", len) > 0) {
            int offset = specialIndexOf(data, "mp4a", len);
            int channel = bigEndian(data + offset + 20, 2); // audio parameter must be set before starting
            int bps = bigEndian(data + offset + 22, 2);     // the aac decoder. There are RAW blocks only in m4a
            int srate = bigEndian(data + offset + 26, 4);   //
//            setBitsPerSample(bps);
//            setChannels(channel);
//            if(!m_M4A_chConfig) m_M4A_chConfig = channel;
//            setSampleRate(srate);
//            m_M4A_sampleRate = srate;
            setBitrate(bps * channel * srate);
//			m_bitRate = bps * channel * srate;
            AUDIO_INFO("ch: %i, bps: %i, sr: %i", channel, bps, srate);
            sprintf(m_chbuf, "%lu", m_bitRate);
            audio_bitrate(m_chbuf);
            if(m_m4aHdr.audioDataPos && m_dataMode == AUDIO_LOCALFILE) {
                m_controlCounter = M4A_AMRDY;
                setFilePos(m_m4aHdr.audioDataPos);
                return 0;
            }
        }
        m_controlCounter = M4A_MOOV;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_controlCounter == M4A_ILST) { // ilst
        const char info[12][6] = {"nam\0", "ART\0", "alb\0", "too\0", "cmt\0", "wrt\0", "tmpo\0", "trkn\0", "day\0", "cpil\0", "aART\0", "gen\0"};
        int        offset = 0;
        // If it's a local file, the metadata has already been read, even if it comes after the audio block.
        // In the event that they are in front of the audio block in a web stream, read them now
        if(!m_f_m4aID3dataAreRead) {
            ps_ptr<char>id3tag = {};
            for(int i = 0; i < 12; i++) {
                offset = specialIndexOf(data, info[i], len, true); // seek info[] with '\0'
                if(offset > 0) {
                    offset += 19;
                    if(*(data + offset) == 0) offset++;
                    char   value[256] = {0};
                    size_t tmp = strlen((const char*)data + offset);
                    if(tmp > 254) tmp = 254;
                    memcpy(value, (data + offset), tmp);
                    value[tmp] = '\0';
                    if(i == 0) id3tag.appendf("Title: %s", value);
                    if(i == 1) id3tag.appendf("Artist: %s", value);
                    if(i == 2) id3tag.appendf("Album: %s", value);
                    if(i == 3) id3tag.appendf("Encoder: %s", value);
                    if(i == 4) id3tag.appendf("Comment: %s", value);
                    if(i == 5) id3tag.appendf("Composer: %s", value);
                    if(i == 6) id3tag.appendf("BPM: %s", value);
                    if(i == 7) id3tag.appendf("Track Number: %s", value);
                    if(i == 8) id3tag.appendf("Year: %s", value);
                    if(i == 9) id3tag.appendf("Compile: %s", value);
                    if(i == 10)id3tag.appendf("Album Artist: %s", value);
                    if(i == 11)id3tag.appendf("Types of: %s", value);
                    if(id3tag.valid()) {
                        if(audio_id3data) audio_id3data(id3tag.get());
                    }
                }
            }
        }
        offset = specialIndexOf(data, "covr", len);
        if(offset > 0){
            m_m4aHdr.picLen = bigEndian(data + offset + 4, 4) - 4;
            m_m4aHdr.picPos = m_m4aHdr.headerSize + offset + 12;
        }
        m_controlCounter = M4A_MOOV;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    uint8_t extLen = 0;
    if(m_controlCounter == M4A_MDAT) {            // mdat
        m_audioDataSize = bigEndian(data, 4); // length of this atom

        // Extended Size
        // 00 00 00 01 6D 64 61 74 00 00 00 00 00 00 16 64
        //        0001  m  d  a  t                    5732

        if(m_audioDataSize == 1){ // Extended Size
            m_audioDataSize = bigEndian(data + 8, 8);
            m_audioDataSize -= 16;
            extLen = 8;
        }
        else m_audioDataSize -= 8;
        AUDIO_INFO("Audio-Length: %i", m_audioDataSize);
        m_m4aHdr.retvalue = 8 + extLen;
        m_m4aHdr.headerSize += 8 + extLen;
        m_controlCounter = M4A_AMRDY; // last step before starting the audio
        return 0;
    }

    if(m_controlCounter == M4A_AMRDY) { // almost ready
        m_audioDataStart = m_m4aHdr.headerSize;
        if(m_dataMode == AUDIO_LOCALFILE) {
            m_contentlength = m_m4aHdr.headerSize + m_audioDataSize; // after this mdat atom there may be other atoms
            if(extLen) m_contentlength -= 16;
            AUDIO_INFO("Content-Length: %lu", (long unsigned int)m_contentlength);
        }
        if(m_m4aHdr.picLen) {
            size_t pos = m_audiofile.position();
            audio_id3image(m_audiofile, m_m4aHdr.picPos, m_m4aHdr.picLen);
            m_audiofile.seek(pos); // the filepointer could have been changed by the user, set it back
        }
        m_controlCounter = M4A_OKAY; // that's all
        return 0;
    }
    // this section should never be reached
    AUDIO_ERROR("error");
    return 0;
}
//###################################################################
size_t Audio::process_m3u8_ID3_Header(uint8_t* packet) {
    uint8_t  ID3version;
    size_t   id3Size;
    bool     m_f_unsync = false, m_f_exthdr = false;
    uint64_t current_timestamp = 0;

    (void)m_f_unsync;        // suppress -Wunused-variable
    (void)current_timestamp; // suppress -Wunused-variable

    if(specialIndexOf(packet, "ID3", 4) != 0) { // ID3 not found
        // log_i("m3u8 file has no mp3 tag");
        return 0; // error, no ID3 signature found
    }
    ID3version = *(packet + 3);
    switch(ID3version) {
        case 2:
            m_f_unsync = (*(packet + 5) & 0x80);
            m_f_exthdr = false;
            break;
        case 3:
        case 4:
            m_f_unsync = (*(packet + 5) & 0x80); // bit7
            m_f_exthdr = (*(packet + 5) & 0x40); // bit6 extended header
            break;
    };
    id3Size = bigEndian(&packet[6], 4, 7); //  ID3v2 size  4 * %0xxxxxxx (shift left seven times!!)
    id3Size += 10;
    // log_i("ID3 framesSize: %i", id3Size);
    // log_i("ID3 version: 2.%i", ID3version);

    if(m_f_exthdr) {
        AUDIO_ERROR("ID3 extended header in m3u8 files not supported");
        return 0;
    }
    // log_i("ID3 normal frames");

    if(specialIndexOf(&packet[10], "PRIV", 5) != 0) { // tag PRIV not found
        AUDIO_ERROR("tag PRIV in m3u8 Id3 Header not found");
        return 0;
    }
    // if tag PRIV exists assume content is "com.apple.streaming.transportStreamTimestamp"
    // a time stamp is expected in the header.

    current_timestamp = (double)bigEndian(&packet[69], 4) / 90000; // seconds

    return id3Size;
}
//###################################################################
uint32_t Audio::stopSong() {
    m_f_lockInBuffer = true; // wait for the decoding to finish
//        uint8_t maxWait = 0;
//        while(m_f_audioTaskIsDecoding) {vTaskDelay(1); maxWait++; if(maxWait > 100) break;} // in case of error wait max 100ms
        uint32_t pos = 0;
        if(m_f_running) {
            m_f_running = false;
            if(m_dataMode == AUDIO_LOCALFILE) {
                pos = getFilePos() - inBufferFilled();
            }
            if(_client->connected()) _client->stop();
        }
        if(m_audiofile) {
            // added this before putting 'm_f_localfile = false' in stopSong(); shoulf never occur....
            AUDIO_INFO("Closing audio file \"%s\"", m_audiofile.name());
            m_audiofile.close();
        }
    uint16_t modereg; 					// Read from mode register
    int i; 							// Loop control
//    m_f_webstream = false;

    sdi_send_fillers(2050); 			//  sdi_send_fillers(vs1053_chunk_size * 54);
    delay(10);
    write_register(SCI_MODE, _BV (SM_SDINEW) | _BV(SM_CANCEL));
    for(i=0; i < 200; i++) {
        sdi_send_fillers(32);
        modereg = read_register(SCI_MODE);  			// Read status
        if((modereg & _BV(SM_CANCEL)) == 0) {
            sdi_send_fillers(2050);
            AUDIO_INFO("Song stopped correctly after %d msec", i * 10);
//            return;
            goto exit;
        }
        delay(10);
    }
    AUDIO_INFO("Song stopped incorrectly!");
    printDetails("after song stopped incorrectly");

exit:
      m_validSamples = 0;
      m_audioCurrentTime = 0;
      m_audioFileDuration = 0;
      m_codec = CODEC_NONE;
      m_dataMode = AUDIO_NONE;
      m_streamType = ST_NONE;
      m_playlistFormat = FORMAT_NONE;
      m_f_lockInBuffer = false;
      return pos;
}
//###################################################################
void Audio::startSong(){
    sdi_send_fillers(2050);
//    sdi_send_fillers(vs1053_chunk_size * 54);
}
//###################################################################
void Audio::printDetails(const char* str){

    if(strlen(str)) AUDIO_INFO(str);

    char decbuf[16][6];
    char hexbuf[16][5];
    char binbuf[16][17];
    uint8_t i;

    const char regName[16][12] = {
        "MODE       ", "STATUS     ", "BASS       ", "CLOCKF     ",
        "DECODE_TIME", "AUDATA     ", "WRAM       ", "WRAMADDR   ",
        "HDAT0      ", "HDAT1      ", "AIADDR     ", "VOL        ",
        "AICTRL0    ", "AICTRL1    ", "AICTRL2    ", "AICTRL3    ",
    };

    for(i=0; i <= SCI_AICTRL3; i++){
        sprintf(hexbuf[i], "%04X", read_register(i));
        sprintf(decbuf[i], "%05d", read_register(i));

        uint16_t tmp   = read_register(i);
        uint16_t shift = 0x8000;
        for(int8_t j = 0; j < 16; j++){
            binbuf[i][j] = (tmp & shift ? '1' : '0');
            shift >>= 1;
        }
        binbuf[i][16] = '\0';
    }

    AUDIO_INFO("REG            dec      bin               hex");
    AUDIO_INFO("-----------  -------  ----------------  -------");

    for(i=0; i <= SCI_AICTRL3; i++){
        AUDIO_INFO("%s   %s   %s   %s", regName[i], decbuf[i], binbuf[i], hexbuf[i]);
    }
}
//###################################################################
uint8_t Audio::printVersion(){
    uint16_t reg = wram_read(0x1E02) & 0xFF;
    return reg;
}

uint32_t Audio::printChipID(){
    uint32_t chipID = 0;
    chipID =  wram_read(0x1E00) << 16;
    chipID += wram_read(0x1E01);
    return chipID;
}
//###################################################################
bool Audio::pauseResume() {
    xSemaphoreTake(mutex_audioTask, 0.3 * configTICK_RATE_HZ);
    bool retVal = false;
    if(m_dataMode == AUDIO_LOCALFILE || m_streamType == ST_WEBSTREAM || m_streamType == ST_WEBFILE) {
        m_f_running = !m_f_running;
        retVal = true;
        if(!m_f_running) {
//            memset(m_outBuff.get(), 0, m_outbuffSize * sizeof(int16_t)); // Clear OutputBuffer
            m_validSamples = 0;
        }
    }
    xSemaphoreGive(mutex_audioTask);
    return retVal;
}
//###################################################################
void Audio::loop() {
    if(!m_f_running) return;

    if(m_playlistFormat != FORMAT_M3U8) { // normal process
        switch(m_dataMode) {
            case AUDIO_LOCALFILE:
                processLocalFile(); break;
            case HTTP_RESPONSE_HEADER:
                if(!parseHttpResponseHeader()) {
                    if(m_f_timeout && m_lVar.count < 3) {m_f_timeout = false; m_lVar.count++; connecttohost(m_lastHost.get());}
                }
                else{
                    m_lVar.count = 0;
                }
                break;
            case AUDIO_PLAYLISTINIT: readPlayListData(); break;
            case AUDIO_PLAYLISTDATA:
                if(m_playlistFormat == FORMAT_M3U) httpPrint(parsePlaylist_M3U());
                if(m_playlistFormat == FORMAT_PLS) httpPrint(parsePlaylist_PLS());
                if(m_playlistFormat == FORMAT_ASX) httpPrint(parsePlaylist_ASX());
                break;
            case AUDIO_DATA:
                if(m_streamType == ST_WEBSTREAM) processWebStream();
                if(m_streamType == ST_WEBFILE)   processWebFile();
                break;
        }
    }
    else { // m3u8 datastream only
        ps_ptr<char> host;
        if(m_lVar.no_host_timer > millis()) {return;}
        switch(m_dataMode) {
            case HTTP_RESPONSE_HEADER:
                if(!parseHttpResponseHeader()) {
                    if(m_f_timeout && m_lVar.count < 3) {m_f_timeout = false; m_lVar.count++; m_f_reset_m3u8Codec = false; connecttohost(m_lastHost.get());}
                }
                else{
                    m_lVar.count = 0;
                    m_f_firstCall  = true;
                }
                break;
            case AUDIO_PLAYLISTINIT: readPlayListData(); break;
            case AUDIO_PLAYLISTDATA:
                host = parsePlaylist_M3U8();
                if(!host.valid()) m_lVar.no_host_cnt++;
                else {m_lVar.no_host_cnt = 0; m_lVar.no_host_timer = millis();}

                if(m_lVar.no_host_cnt == 2){m_lVar.no_host_timer = millis() + 2000;} // no new url? wait 2 seconds
                if(host.valid()) { // host contains the next playlist URL
                    httpPrint(host.get());
                    m_dataMode = HTTP_RESPONSE_HEADER;
                }
                else { // host == NULL means connect to m3u8 URL
                    if(m_lastM3U8host.valid()) {m_f_reset_m3u8Codec = false; httpPrint(m_lastM3U8host.get());}
                    else                       {httpPrint(m_lastHost.get());}      // if url has no first redirection
                    m_dataMode = HTTP_RESPONSE_HEADER;               // we have a new playlist now
                }
                break;
            case AUDIO_DATA:
                if(m_f_ts) { processWebStreamTS(); } // aac or aacp with ts packets
                else { processWebStreamHLS(); }      // aac or aacp normal stream

                if(m_f_continue) { // at this point m_f_continue is true, means processWebStream() needs more data
                    m_dataMode = AUDIO_PLAYLISTDATA;
                    m_f_continue = false;
                }
                break;
        }
    }
}
//###################################################################
bool Audio::readPlayListData() {
    if(m_dataMode != AUDIO_PLAYLISTINIT) return false;
    if(_client->available() == 0) return false;

    uint32_t chunksize = 0;
    uint8_t  readedBytes = 0;
    if(m_f_chunked) chunksize = readChunkSize(&readedBytes);

    // reads the content of the playlist and stores it in the vector m_contentlength
    // m_contentlength is a table of pointers to the lines
    ps_ptr<char>pl;
    uint32_t ctl = 0;
    int      lines = 0;
    // delete all memory in m_playlistContent
    if(m_playlistFormat == FORMAT_M3U8 && !psramFound()) { AUDIO_ERROR("m3u8 playlists requires PSRAM enabled!"); }
    vector_clear_and_shrink(m_playlistContent);
    while(true) { // outer while

        uint32_t ctime = millis();
        uint32_t timeout = 2000; // ms
        pl.alloc(1024,"pl"); pl.clear(); // playlistLine

        while(true) { // inner while
            uint16_t pos = 0;
            while(_client->available()) { // super inner while :-))
                pl[pos] = _client->read();
                ctl++;
                if(pl[pos] == '\n') {
                    pl[pos] = '\0';
                    pos++;
                    break;
                }
                //    if(pl[pos] == '&' ) {pl[pos] = '\0'; pos++; break;}
                if(pl[pos] == '\r') {
                    pl[pos] = '\0';
                    pos++;
                    continue;
                    ;
                }
                pos++;
                if(pos == 1022) {
                    pos--;
                    continue;
                }
                if(pos == 509) { pl[pos] = '\0'; }
                if(ctl == chunksize) {
                    pl[pos] = '\0';
                    break;
                }
                if(ctl == m_contentlength) {
                    pl[pos] = '\0';
                    break;
                }
            }
            if(ctl == chunksize) break;
            if(ctl == m_contentlength) break;
            if(pos) {
                pl[pos] = '\0';
                break;
            }

            if(ctime + timeout < millis()) {
                AUDIO_ERROR("timeout");
                for(int i = 0; i < m_playlistContent.size(); i++) AUDIO_ERROR("pl%i = %s", i, m_playlistContent[i].get());
                goto exit;
            }
        } // inner while

        if(pl.starts_with_icase("<!DOCTYPE")) {
            AUDIO_ERROR("url is a webpage!");
            goto exit;
        }
        if(pl.starts_with_icase("<html")) {
            AUDIO_ERROR("url is a webpage!");
            goto exit;
        }
        if(pl.size() > 0) m_playlistContent.emplace_back(std::move(pl));

        if(m_playlistContent.size() && m_playlistContent.size() % 1000 == 0) { AUDIO_INFO("current playlist line: %lu", (long unsigned)m_playlistContent.size()); }
        // termination conditions
        // 1. The http response header returns a value for contentLength -> read chars until contentLength is reached
        // 2. no contentLength, but Transfer-Encoding:chunked -> compute chunksize and read until chunksize is reached
        // 3. no chunksize and no contentlengt, but Connection: close -> read all available chars
        if(ctl == m_contentlength) {
            while(_client->available()) _client->read();
            break;
        } // read '\n\n' if exists
        if(ctl == chunksize) {
            while(_client->available()) _client->read();
            break;
        }
        if(!_client->connected() && _client->available() == 0) break;

    } // outer while
    lines = m_playlistContent.size();
    for(int i = 0; i < lines; i++) { // print all string in first vector of 'arr'
    //    log_w("pl=%i \"%s\"", i, m_playlistContent[i]);
    }
    m_dataMode = AUDIO_PLAYLISTDATA;
    return true;

exit:
    vector_clear_and_shrink(m_playlistContent);
    m_f_running = false;
    m_dataMode = AUDIO_NONE;
    return false;
}
//##################################################################
const char* Audio::parsePlaylist_M3U() {
    uint8_t lines = m_playlistContent.size();
    int     pos = 0;
    char*   host = nullptr;

    for(int i = 0; i < lines; i++) {
        if(m_playlistContent[i].contains("#EXTINF:")) { // Info?
            pos = m_playlistContent[i].index_of(",");        // Comma in this line?
            if(pos > 0) {
                // Show artist and title if present in metadata
                AUDIO_INFO(m_playlistContent[i].get() + pos + 1);
            }
            continue;
        }
        if(m_playlistContent[i].starts_with("#")) { // Commentline?
            continue;
        }

        pos = m_playlistContent[i].index_of("http://:@", 0); // ":@"??  remove that!
        if(pos >= 0) {
            AUDIO_INFO("Entry in playlist found: %s", (m_playlistContent[i].get() + pos + 9));
            host = m_playlistContent[i].get() + pos + 9;
            break;
        }
        // AUDIO_INFO("Entry in playlist found: %s", pl);
        pos = m_playlistContent[i].index_of("http", 0); // Search for "http"
        if(pos >= 0) {                                  // Does URL contain "http://"?
                                                        //    AUDIO_ERROR(""%s pos=%i", m_playlistContent[i], pos);
            host = m_playlistContent[i].get() + pos;          // Yes, set new host
            break;
        }
    }
    //    vector_clear_and_shrink(m_playlistContent);
    return host;
}
//###################################################################
const char* Audio::parsePlaylist_PLS() {
    uint8_t lines = m_playlistContent.size();
    int     pos = 0;
    char*   host = nullptr;

    for(int i = 0; i < lines; i++) {
        if(i == 0) {
            if(m_playlistContent[0].strlen() == 0) goto exit;     // empty line
            if(!m_playlistContent[0].starts_with("[playlist]")) { // first entry in valid pls
                m_dataMode = HTTP_RESPONSE_HEADER;                // pls is not valid
                AUDIO_INFO("pls is not valid, switch to HTTP_RESPONSE_HEADER");
                goto exit;
            }
            continue;
        }
        if(m_playlistContent[i].starts_with("File1")) {
            if(host) continue;                              // we have already a url
            pos = m_playlistContent[i].index_of("http", 0); // File1=http://streamplus30.leonex.de:14840/;
            if(pos >= 0) {                                  // yes, URL contains "http"?
                host = m_playlistContent[i].get() + pos;          // Now we have an URL for a stream in host.
            }
            continue;
        }
        if(m_playlistContent[i].starts_with("Title1")) { // Title1=Antenne Tirol
            const char* plsStationName = (m_playlistContent[i].get() + 7);
            if(audio_showstation) audio_showstation(plsStationName);
            AUDIO_INFO("StationName: \"%s\"", plsStationName);
            continue;
        }
        if(m_playlistContent[i].starts_with("Length1")) { continue; }
        if(m_playlistContent[i].contains("Invalid username")) { // Unable to access account:
            goto exit;                                          // Invalid username or password
        }
    }
    return host;

exit:
    m_f_running = false;
    stopSong();
    vector_clear_and_shrink(m_playlistContent);
    m_dataMode = AUDIO_NONE;
    return nullptr;
}
//##################################################################
const char* Audio::parsePlaylist_ASX() { // Advanced Stream Redirector
    uint8_t lines = m_playlistContent.size();
    bool    f_entry = false;
    int     pos = 0;
    char*   host = nullptr;

    for(int i = 0; i < lines; i++) {
        int p1 = m_playlistContent[i].index_of("<", 0);
        int p2 = m_playlistContent[i].index_of(">", 1);
        if(p1 >= 0 && p2 > p1) { // #196 set all between "< ...> to lowercase
            for(uint8_t j = p1; j < p2; j++) { m_playlistContent[i][j] = toLowerCase(m_playlistContent[i][j]); }
        }
        if(m_playlistContent[i].contains("<entry>")) f_entry = true; // found entry tag (returns -1 if not found)
        if(f_entry) {
            if(m_playlistContent[i].contains("ref href")) { //  <ref href="http://87.98.217.63:24112/stream" />
                pos = m_playlistContent[i].index_of("http", 0);
                if(pos > 0) {
                    host = (m_playlistContent[i].get() + pos); // http://87.98.217.63:24112/stream" />
                    int pos1 = indexOf(host, "\"", 0);   // http://87.98.217.63:24112/stream
                    if(pos1 > 0) host[pos1] = '\0';      // Now we have an URL for a stream in host.
                }
            }
        }
        pos = m_playlistContent[i].index_of("<title>", 0);
        if(pos >= 0) {
            char* plsStationName = (m_playlistContent[i].get() + pos + 7); // remove <Title>
            pos = indexOf(plsStationName, "</", 0);
            if(pos >= 0) {
                *(plsStationName + pos) = 0; // remove </Title>
            }
            if(audio_showstation) audio_showstation(plsStationName);
            AUDIO_INFO("StationName: \"%s\"", plsStationName);
        }

        if(m_playlistContent[i].starts_with("http") && !f_entry) { // url only in asx
            host = m_playlistContent[i].get();
        }
    }
    return host;
}
//###################################################################
ps_ptr<char> Audio::parsePlaylist_M3U8() {
    // example: audio chunks
    // #EXTM3U
    // #EXT-X-TARGETDURATION:10
    // #EXT-X-MEDIA-SEQUENCE:163374040
    // #EXT-X-DISCONTINUITY
    // #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
    // http://n3fa-e2.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/main/163374038.aac
    // #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
    // http://n3fa-e2.revma.ihrhls.com/zc7729/63_sdtszizjcjbz02/main/163374039.aac

    if(!m_lastHost.valid())      {AUDIO_ERROR("m_lastHost is NULL");     return {};} // guard

    uint8_t lines = m_playlistContent.size();
    bool    f_haveRedirection = false;
    char    llasc[21]; // uint64_t max = 18,446,744,073,709,551,615  thats 20 chars + \0

    if(lines){
        bool addNextLine = false;
        vector_clear_and_shrink(m_linesWithSeqNrAndURL);
        vector_clear_and_shrink(m_linesWithEXTINF);
        for(uint8_t i = 0; i < lines; i++) {
            //  log_w("pl%i = %s", i, m_playlistContent[i].get());
            if(m_playlistContent[i].starts_with("#EXT-X-STREAM-INF:")) { f_haveRedirection = true; /*AUDIO_ERROR("we have a redirection");*/}
            if(addNextLine) {
                addNextLine = false;
                // size_t len = strlen(linesWithSeqNr[idx].get()) + strlen(m_playlistContent[i].get()) + 1;
                m_linesWithSeqNrAndURL.emplace_back().clone_from(m_playlistContent[i]);
            }
            if(startsWith(m_playlistContent[i].get(), "#EXTINF:")) {
                m_linesWithEXTINF.emplace_back().clone_from(m_playlistContent[i]);
                addNextLine = true;
            }
        }
    }

    for(int i = 0; i < m_linesWithSeqNrAndURL.size(); i++) {/*log_w("%s", m_linesWithSeqNrAndURL[i].get())*/;}
    for(int i = 0; i < m_linesWithEXTINF.size(); i++)      {/*log_w("%s", m_linesWithEXTINF[i].get());*/ showstreamtitle(m_linesWithEXTINF[i].get());}

    if(f_haveRedirection) {
        ps_ptr<char> ret = m3u8redirection(&m_m3u8Codec);
        m_lastM3U8host.assign(ret.get());
        vector_clear_and_shrink(m_playlistContent);
        return {};
    }

    if(m_f_firstM3U8call) {
        m_f_firstM3U8call = false;
        m_pplM3U8.xMedSeq = 0;
        m_pplM3U8.f_mediaSeq_found = false;
    }

    if(!m_pplM3U8.f_mediaSeq_found) {m_pplM3U8.f_mediaSeq_found = m3u8_findMediaSeqInURL(m_linesWithSeqNrAndURL, &m_pplM3U8.xMedSeq); AUDIO_INFO("MediaSequenceNumber: %lli", m_pplM3U8.xMedSeq);}
    if(m_codec == CODEC_NONE) {m_codec = CODEC_AAC; if(m_m3u8Codec == CODEC_MP3) m_codec = CODEC_MP3;}  // if we have no redirection
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    boolean f_EXTINF_found = false;

    if(lines) {
        for(uint16_t i = 0; i < m_linesWithSeqNrAndURL.size(); i++) {

            f_EXTINF_found = true;

            ps_ptr<char> tmp;
            if(!m_linesWithSeqNrAndURL[i].starts_with("http")) {

                //  playlist:   http://station.com/aaa/bbb/xxx.m3u8
                //  chunklist:  http://station.com/aaa/bbb/ddd.aac
                //  result:     http://station.com/aaa/bbb/ddd.aac

                if(m_lastM3U8host.valid()) {
                    tmp.clone_from(m_lastM3U8host, "tmp");
                }
                else {
                    tmp.clone_from(m_lastHost, "tmp");
                }
                if(m_linesWithSeqNrAndURL[i][0] != '/'){

                    //  playlist:   http://station.com/aaa/bbb/xxx.m3u8  // tmp
                    //  chunklist:  ddd.aac                              // m_linesWithSeqNrAndURL[i]
                    //  result:     http://station.com/aaa/bbb/ddd.aac   // m_linesWithSeqNrAndURL[i]

                    int idx = tmp.last_index_of('/');
                    tmp[idx  + 1] = '\0';
                    tmp.append(m_linesWithSeqNrAndURL[i].get());
                }
                else{

                    //  playlist:   http://station.com/aaa/bbb/xxx.m3u8
                    //  chunklist:  /aaa/bbb/ddd.aac
                    //  result:     http://station.com/aaa/bbb/ddd.aac
                    int idx = tmp.index_of('/', 8);
                    tmp[idx] = '\0';
                    tmp.append(m_linesWithSeqNrAndURL[i].get());
                }
            }
            else {tmp.append(m_linesWithSeqNrAndURL[i].get());}

            if(m_pplM3U8.f_mediaSeq_found) {
                lltoa(m_pplM3U8.xMedSeq, llasc, 10);
                if(tmp.contains(llasc)) {
                    m_playlistURL.insert(m_playlistURL.begin(), std:: move(tmp));
                    m_pplM3U8.xMedSeq++;
                }
                else{
                    lltoa(m_pplM3U8.xMedSeq + 1, llasc, 10);
                    if(tmp.contains(llasc)) {
                        m_playlistURL.insert(m_playlistURL.begin(), std::move(tmp));
                        log_w("mediaseq %llu skipped", m_pplM3U8.xMedSeq);
                        m_pplM3U8.xMedSeq+= 2;
                    }
                }
            }
            else { // without mediaSeqNr, with hash
                uint32_t hash = simpleHash(tmp.get());
                if(m_hashQueue.size() == 0) {
                    m_hashQueue.insert(m_hashQueue.begin(), hash);
                    m_playlistURL.insert(m_playlistURL.begin(), std::move(tmp));
                }
                else {
                    bool known = false;
                    for(int i = 0; i < m_hashQueue.size(); i++) {
                        if(hash == m_hashQueue[i]) {
                            // log_i("file already known %s", tmp);
                            known = true;
                        }
                    }
                    if(!known) {
                        m_hashQueue.insert(m_hashQueue.begin(), hash);
                        m_playlistURL.insert(m_playlistURL.begin(), std::move(tmp));
                    }
                }
                if(m_hashQueue.size() > 20) m_hashQueue.pop_back();
            }
            continue;
        }
        vector_clear_and_shrink(m_playlistContent); // clear after reading everything, m_playlistContent.size is now 0
    }
    if(m_playlistURL.size() > 0) {
        ps_ptr<char>m_playlistBuff;
        if(m_playlistURL[m_playlistURL.size() - 1].valid()) {
            m_playlistBuff.append(m_playlistURL[m_playlistURL.size() - 1].get());
            m_playlistURL.pop_back();
            m_playlistURL.shrink_to_fit();
        }
        // log_i("now playing %s", m_playlistBuff);
        if(endsWith(m_playlistBuff.get(), "ts")) m_f_ts = true;
        if(m_playlistBuff.contains(".ts?") > 0)  m_f_ts = true;
        return m_playlistBuff;
    }
    else {
        if(f_EXTINF_found) {
            if(m_pplM3U8.f_mediaSeq_found) {
                if(m_playlistContent.size() == 0) return {};

                uint64_t mediaSeq;
                m_pplM3U8.f_mediaSeq_found = m3u8_findMediaSeqInURL(m_linesWithSeqNrAndURL, &mediaSeq);
                if(!m_pplM3U8.f_mediaSeq_found) {
                    AUDIO_ERROR("xMediaSequence not found");
                    httpPrint(m_lastHost.get());
                }
                if(mediaSeq < m_pplM3U8.xMedSeq) {
                    uint64_t diff = m_pplM3U8.xMedSeq - mediaSeq;
                    if(diff < 10) { ; }
                    else {
                        if(m_playlistContent.size() > 0) {
                            for(int j = 0; j < lines; j++) {
                                // log_i("lines %i, %s", lines, m_playlistContent[j]);
                            }
                        }
                        else { ; }

                        if(m_playlistURL.size() > 0) {
                            for(int j = 0; j < m_playlistURL.size(); j++) {
                                // log_i("m_playlistURL lines %i, %s", j, m_playlistURL[j]);
                            }
                        }
                        else { ; }

                        if(m_playlistURL.size() == 0) {
                            m_f_reset_m3u8Codec = false;
                            httpPrint(m_lastHost.get());
                        }
                    }
                }
                else {
                    if(mediaSeq != UINT64_MAX) { AUDIO_ERROR("err, %u packets lost from %u, to %u", mediaSeq - m_pplM3U8.xMedSeq, m_pplM3U8.xMedSeq, mediaSeq); }
                    m_pplM3U8.xMedSeq = mediaSeq;
                }
            } // f_medSeq_found
        }
    }
    return {};
}
//##################################################################
ps_ptr<char>Audio::m3u8redirection(uint8_t* codec) {
    // example: redirection
    // #EXTM3U
    // #EXT-X-STREAM-INF:BANDWIDTH=117500,AVERAGE-BANDWIDTH=117000,CODECS="mp4a.40.2"
    // 112/playlist.m3u8?hlssid=7562d0e101b84aeea0fa35f8b963a174
    // #EXT-X-STREAM-INF:BANDWIDTH=69500,AVERAGE-BANDWIDTH=69000,CODECS="mp4a.40.5"
    // 64/playlist.m3u8?hlssid=7562d0e101b84aeea0fa35f8b963a174
    // #EXT-X-STREAM-INF:BANDWIDTH=37500,AVERAGE-BANDWIDTH=37000,CODECS="mp4a.40.29"
    // 32/playlist.m3u8?hlssid=7562d0e101b84aeea0fa35f8b963a174

    if(!m_lastHost.valid()) {AUDIO_ERROR("m_lastHost is empty"); return {};} // guard
    const char codecString[9][11]={
        "mp4a.40.34", // mp3 stream
        "mp4a.40.01", // AAC Main
        "mp4a.40.2",  // MPEG-4 AAC LC
        "mp4a.40.02", // MPEG-4 AAC LC, leading 0 for Aud-OTI compatibility
        "mp4a.40.29", // MPEG-4 HE-AAC v2 (AAC LC + SBR + PS)
        "mp4a.40.42", // xHE-AAC
        "mp4a.40.5",  // MPEG-4 HE-AAC v1 (AAC LC + SBR)
        "mp4a.40.05", // MPEG-4 HE-AAC v1 (AAC LC + SBR), leading 0 for Aud-OTI compatibility
        "mp4a.67",    // MPEG-2 AAC LC
    };

    uint16_t choosenLine = 0;
    uint16_t plcSize = m_playlistContent.size();
    int8_t   cS = 100;

    for(uint16_t i = 0; i < plcSize; i++) { // looking for lowest codeString
        if(m_playlistContent[i].contains("CODECS=\"mp4a")){
            for (uint8_t j = 0; j < 9; j++) {
                if (m_playlistContent[i].contains(codecString[j])) {
                    if (j < cS) {
                        cS = j;
                        choosenLine = i;
                    }
                }
            }
        }
    }
    if (cS == 0)            *codec = CODEC_MP3;
    else if (cS < 100)      *codec = CODEC_AAC;



    if(cS == 100) {                             // "mp4a.xx.xx" not found
        *codec = CODEC_AAC;                     // assume AAC
        for(uint16_t i = 0; i < plcSize; i++) { // we have no codeString, looking for "http"
            if(m_playlistContent[i].contains("#EXT-X-STREAM-INF")){
                choosenLine = i;
            }
        }
    }

    choosenLine++; // next line is the redirection url

    ps_ptr<char> result = {};
    ps_ptr<char>line; line.clone_from(m_playlistContent[choosenLine]);

    if(line.starts_with("../")){
        // ../../2093120-b/RISMI/stream01/streamPlaylist.m3u8
        while (line.starts_with("../")){
            line.remove_prefix("../");
        }
        result.clone_from(m_lastHost);
        int idx = result.last_index_of('/');
        if(idx > 0 && (size_t)idx != result.strlen() - 1) result.truncate_at((size_t)idx + 1);
        result.append(line.get());
    }
    else if(!line.starts_with_icase("http")) {

        // http://arcast.com.ar:1935/radio/radionar.stream/playlist.m3u8               m_lastHost
        // chunklist_w789468022.m3u8                                                   line
        // http://arcast.com.ar:1935/radio/radionar.stream/chunklist_w789468022.m3u8   --> result

        result.clone_from(m_lastHost);
        int pos = m_lastHost.last_index_of('/');
        if(pos > 0){
            result.truncate_at((size_t)pos + 1);
            result.append(line.get());
        }
    }
    else {
        result.clone_from(line);
    }

    return result; // it's a redirection, a new m3u8 playlist
}
//##################################################################
bool Audio::m3u8_findMediaSeqInURL(std::vector<ps_ptr<char>>&linesWithSeqNr, uint64_t* mediaSeqNr) { // We have no clue what the media sequence is

    // example: linesWithSeqNr
    // p:,<p:,<10.032,media_w630738364_405061.mp3
    // p:,<p:,<9.952,media_w630738364_405062.mp3
    // p:,<p:,<10.031,media_w630738364_405063.mp3

    if(linesWithSeqNr.size() < 3) return false;
    //     for (uint16_t i = 0; i < linesWithSeqNr.size(); i++) {
    //         log_w("linesWithSeqNr[%i] = %s", i, linesWithSeqNr[i].get());
    //     }

    uint16_t pos;
    std::array<std::vector<int64_t>, 3>numbers;
    for(int i = 0; i < 3; i++){
        pos = 0;
        for(pos = 0; pos < linesWithSeqNr[i].strlen(); pos++){
            if(isdigit(linesWithSeqNr[i][pos])){
                numbers[i].push_back(strtoull(linesWithSeqNr[i].get() + pos, nullptr, 10));
                // log_w("%lli", strtoull(linesWithSeqNr[i].get() + pos, nullptr, 10));
                while(pos < linesWithSeqNr[i].strlen()){ // skip to next number or line end
                    pos++;
                    if(!isdigit(linesWithSeqNr[i][pos])) break;
                }
            }
        }
    }

    // There must be three valid lines with a sequencer.Then the integers are sought for each line.For the example, these are:
    // numbers[0] 10 32 630738364 405061
    // numbers[1] 9 952 630738364 405062
    // numbers[2] 10 31 630738364 405063
    // then the absolute value is formed (-234 is also possible if there is a hyphen) and it is searched for three consecutive numbers

    uint16_t idx = 0;
    while(idx < numbers[0].size()){
        // log_w("%lli, %lli, %lli", abs(numbers[0][idx]), abs(numbers[1][idx]), abs(numbers[2][idx]));
        if(abs(numbers[0][idx]) + 1 == abs(numbers[1][idx]) && abs(numbers[1][idx]) + 1 == abs(numbers[2][idx])){
            *mediaSeqNr = abs(numbers[0][idx]);
            return true;
        }
        idx++;
    }
    return false;
}
//##################################################################
void Audio::processLocalFile() {
    if(!(m_audiofile && m_f_running && m_dataMode == AUDIO_LOCALFILE)) return; // guard

    m_prlf.maxFrameSize = InBuff.getMaxBlockSize(); // every mp3/aac frame is not bigger maxFrameSize = InBuff.getMaxBlockSize(); // every mp3/aac frame is not bigger
    m_prlf.availableBytes = 0;
    m_prlf.bytesAddedToBuffer = 0;
    m_prlf.offset = 0;

    if(m_f_firstCall) { // runs only one time per connection, prepare for start
        m_f_firstCall = false;
        m_f_stream = false;
        m_prlf.audioHeaderFound = false;
        m_prlf.newFilePos = 0;
        m_prlf.byteCounter = 0;
        m_prlf.ctime = millis();
//        if(m_codec == CODEC_M4A) seek_m4a_stsz(); // determine the pos of atom stsz
//        if(m_codec == CODEC_M4A) seek_m4a_ilst(); // looking for metadata
        m_audioDataSize = 0;
        m_audioDataStart = 0;
        m_f_allDataReceived = false;
        m_prlf.timeout = 8000; // ms
        return;
    }

    if(m_resumeFilePos >= 0 && m_prlf.newFilePos == 0) { // we have a resume file position
        if(!m_prlf.audioHeaderFound){log_w("timeOffset not possible"); m_resumeFilePos = -1; return;}
        if(m_resumeFilePos <  (int32_t)m_audioDataStart) m_resumeFilePos = m_audioDataStart;
        if(m_resumeFilePos >= (int32_t)m_audioDataStart + m_audioDataSize) {m_resumeFilePos = 0; /*goto exit;*/}

        m_f_lockInBuffer = true;                          // lock the buffer, the InBuffer must not be re-entered in playAudioData()
            while(m_f_audioTaskIsDecoding) vTaskDelay(1); // We can't reset the InBuffer while the decoding is in progress
            InBuff.resetBuffer();
        m_f_lockInBuffer = false;
        m_prlf.newFilePos = m_resumeFilePos;
        m_audiofile.seek(m_prlf.newFilePos);
        m_f_allDataReceived = false;
        m_prlf.byteCounter = m_prlf.newFilePos;
        return;
    }

    m_prlf.availableBytes = InBuff.writeSpace();
    m_prlf.bytesAddedToBuffer = m_audiofile.read(InBuff.getWritePtr(), m_prlf.availableBytes);
    if(m_prlf.bytesAddedToBuffer > 0) {m_prlf.byteCounter += m_prlf.bytesAddedToBuffer; InBuff.bytesWritten(m_prlf.bytesAddedToBuffer);}
    if(m_audioDataSize && m_prlf.byteCounter >= m_audioDataSize){if(!m_f_allDataReceived) m_f_allDataReceived = true;}
    if(!m_audioDataSize && m_prlf.byteCounter == m_fileSize){if(!m_f_allDataReceived) m_f_allDataReceived = true;}
    // AUDIO_ERROR("byteCounter %u >= m_audioDataSize %u, m_f_allDataReceived % i", byteCounter, m_audioDataSize, m_f_allDataReceived);

    if(m_prlf.newFilePos) { // we have a new file position
        if(InBuff.bufferFilled() < InBuff.getMaxBlockSize()) return;
//        if(m_codec == CODEC_OPUS || m_codec == CODEC_VORBIS) {if(InBuff.bufferFilled() < 0xFFFF) return;} // ogg frame <= 64kB
//        if(m_codec == CODEC_WAV)   {while((m_resumeFilePos % 4) != 0){m_resumeFilePos++; m_prlf.offset++; if(m_resumeFilePos >= m_fileSize) goto exit;}}  // must divisible by four
        if(m_codec == CODEC_MP3)   {m_prlf.offset = mp3_correctResumeFilePos();  if(m_prlf.offset == -1) goto exit; }
        if(m_codec == CODEC_FLAC)  {m_prlf.offset = flac_correctResumeFilePos(); if(m_prlf.offset == -1) goto exit; }
//        if(m_codec == CODEC_M4A)   {m_prlf.offset = m4a_correctResumeFilePos();  if(m_prlf.offset == -1) goto exit;}
//        if(m_codec == CODEC_VORBIS){m_prlf.offset = ogg_correctResumeFilePos();  if(m_prlf.offset == -1) goto exit; }
//        if(m_codec == CODEC_OPUS)  {m_prlf.offset = ogg_correctResumeFilePos();  if(m_prlf.offset == -1) goto exit; }
        m_haveNewFilePos  = m_prlf.newFilePos + m_prlf.offset - m_audioDataStart;
        m_sumBytesDecoded = m_prlf.newFilePos + m_prlf.offset - m_audioDataStart;
        m_prlf.newFilePos = 0;
        m_resumeFilePos = -1;
        InBuff.bytesWasRead(m_prlf.offset);
        m_prlf.byteCounter += m_prlf.offset;
    }

    if(!m_f_stream) {
        if(m_codec == CODEC_OGG) { // AUDIO_ERROR("determine correct codec here");
            uint8_t codec = determineOggCodec(InBuff.getReadPtr(), m_prlf.maxFrameSize);
            if     (codec == CODEC_FLAC)   { m_codec = CODEC_FLAC; AUDIO_INFO("format is flac"); return;}
//            else if(codec == CODEC_OPUS)   { m_codec = CODEC_OPUS; AUDIO_INFO("format is opus"); return;}
//            else if(codec == CODEC_VORBIS) { m_codec = CODEC_VORBIS; AUDIO_INFO("format is vorbis"); return;}
            else                           {stopSong(); return;}
        }
        if(m_controlCounter != 100) {
            if((millis() - m_prlf.ctime) > m_prlf.timeout) {
                AUDIO_ERROR("audioHeader reading timeout");
                m_f_running = false;
                goto exit;
            }
            if(InBuff.bufferFilled() > m_prlf.maxFrameSize || (InBuff.bufferFilled() == m_fileSize) || m_f_allDataReceived) { // at least one complete frame or the file is smaller
                InBuff.bytesWasRead(readAudioHeader(InBuff.getMaxAvailableBytes()));
            }
            if(m_controlCounter == 100){
                if(m_audioDataStart > 0){ m_prlf.audioHeaderFound = true; }
                if(!m_audioDataSize) m_audioDataSize = m_fileSize;
                m_prlf.byteCounter = getFilePos();
            }
            return;
        }
        else {
            m_f_stream = true;
            AUDIO_INFO("stream ready");
        }
    }

    if(m_fileStartPos > 0){
        setFilePos(m_fileStartPos);
        m_fileStartPos = -1;
    }

    // end of file reached? - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_eof){ // m_f_eof and m_f_ID3v1TagFound will be set in playAudioData()
        if(m_f_ID3v1TagFound) readID3V1Tag();
exit:
        ps_ptr<char>afn; // audio file name
        if(m_audiofile) afn.assign(m_audiofile.name()); // store temporary the name
        stopSong();
        m_audioCurrentTime = 0;
        m_audioFileDuration = 0;
        m_resumeFilePos = -1;
        m_haveNewFilePos = 0;
        m_codec = CODEC_NONE;

        if(afn.valid()) {
            if(audio_eof_mp3) audio_eof_mp3(afn.c_get());
            AUDIO_INFO("End of file \"%s\"", afn.c_get());
        }
        return;
    }
}
//##################################################################
void Audio::processWebStream() {
    if(m_dataMode != AUDIO_DATA) return; // guard

    m_pwst.maxFrameSize = InBuff.getMaxBlockSize(); // every mp3/aac frame is not bigger
    m_pwst.availableBytes = 0; // available from stream
    m_pwst.f_clientIsConnected = _client->connected();

    // first call, set some values to default  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        m_f_stream = false;
        m_pwst.chunkSize = 0;
        m_metacount = m_metaint;
        m_pwst.f_skipCRLF = false;
        m_f_allDataReceived = false;
        readMetadata(0, true);
    }
    if(m_pwst.f_clientIsConnected) m_pwst.availableBytes = _client->available(); // available from stream

    // chunked data tramsfer - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_chunked && m_pwst.availableBytes > 0) {
        uint8_t readedBytes = 0;
        if(!m_pwst.chunkSize){
            if(m_pwst.f_skipCRLF){
                if(_client->available() < 2) { // avoid getting out of sync
                    if(!m_f_tts) AUDIO_INFO("webstream chunked: not enough bytes available for skipCRLF");
                    return;
                }
                int a =_client->read(); if(a != 0x0D) log_w("chunk count error, expected: 0x0D, received: 0x%02X", a); // skipCR
                int b =_client->read(); if(b != 0x0A) log_w("chunk count error, expected: 0x0A, received: 0x%02X", b); // skipLF
                m_pwst.f_skipCRLF = false;
            }
            if(_client->available()){
                m_pwst.chunkSize = readChunkSize(&readedBytes);
                if(m_pwst.chunkSize > 0) {
                    m_pwst.f_skipCRLF = true; // skip next CRLF
                }
                // log_w("chunk size: %d", chunkSize);
            }
        }
        m_pwst.availableBytes = min(m_pwst.availableBytes, m_pwst.chunkSize);
    }

    // we have metadata  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_metadata && m_pwst.availableBytes) {
        if(m_metacount == 0) {
            int metaLen = readMetadata(m_pwst.availableBytes);
            m_pwst.chunkSize -= metaLen; // reduce chunkSize by metadata length
            return;
        }
        m_pwst.availableBytes = min(m_pwst.availableBytes, m_metacount);
    }

    // if the buffer is often almost empty issue a warning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_stream) {
        if(!m_f_allDataReceived) if(streamDetection(m_pwst.availableBytes)) return;
        if(!m_pwst.f_clientIsConnected) {if(m_f_tts && !m_f_allDataReceived) m_f_allDataReceived = true;} // connection closed (OpenAi)
    }

    // buffer fill routine - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_pwst.availableBytes) {
        m_pwst.availableBytes = min(m_pwst.availableBytes, (uint32_t)InBuff.writeSpace());
        int32_t bytesAddedToBuffer = _client->read(InBuff.getWritePtr(), m_pwst.availableBytes);
        if(bytesAddedToBuffer > 0) {
            if(m_f_metadata) m_metacount -= bytesAddedToBuffer;
            if(m_f_chunked) m_pwst.chunkSize -= bytesAddedToBuffer;
            InBuff.bytesWritten(bytesAddedToBuffer);
        }
    }

    // start audio decoding - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(InBuff.bufferFilled() > m_pwst.maxFrameSize && !m_f_stream) { // waiting for buffer filled
        if(m_codec == CODEC_OGG) { // log_i("determine correct codec here");
            uint8_t codec = determineOggCodec(InBuff.getReadPtr(), m_pwst.maxFrameSize);
            if(codec == CODEC_FLAC) {m_codec = codec; AUDIO_INFO("format is flac");}
            if(codec == CODEC_VORBIS) {m_codec = codec; AUDIO_INFO("format is vorbis");}
            if(codec == CODEC_OPUS) {m_codec = codec; AUDIO_INFO("format is opus"); AUDIO_ERROR("can't play OPUS yet");}
        }
        AUDIO_INFO("stream ready");
        m_f_stream = true;  // ready to play the audio data
    }

    if(m_f_eof) {
        AUDIO_INFO("End of webstream: \"%s\"", m_lastHost.c_get());
        if(audio_eof_stream) audio_eof_stream(m_lastHost.c_get());
        stopSong();
    }
}
//###################################################################
void Audio::processWebFile() {
    if(!m_lastHost.valid()) {AUDIO_ERROR("m_lastHost is empty"); return;}  // guard
    m_pwf.maxFrameSize = InBuff.getMaxBlockSize();        // every mp3/aac frame is not bigger
    m_pwf.f_clientIsConnected = _client;                  // if _client is Nullptr, we are not connected

    // first call, set some values to default - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        m_pwf.f_waitingForPayload = true;
        m_t0 = millis();
        m_pwf.byteCounter = 0;
        m_pwf.chunkSize = 0;
        m_pwf.audioDataCount = 0;
        m_f_stream = false;
        m_audioDataSize = m_contentlength;
        m_webFilePos = 0;
        m_controlCounter = 0;
        m_f_allDataReceived = false;
    }

    uint32_t availableBytes = 0;

    if(m_pwf.f_clientIsConnected) availableBytes = _client->available(); // available from stream
    else AUDIO_ERROR("client not connected");
    // waiting for payload - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_pwf.f_waitingForPayload){
        if(availableBytes == 0){
            if(m_t0 + 3000 < millis()) {
                m_pwf.f_waitingForPayload = false;
                AUDIO_ERROR("no payload received, timeout");
                stopSong();
                m_f_running = false;
            }
            return;
        }
        else {
            m_pwf.f_waitingForPayload = false;
        }
    }

    // chunked data tramsfer - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_chunked && availableBytes) {
        uint8_t readedBytes = 0;
        if(m_f_chunked && m_contentlength == m_pwf.byteCounter) {
            if(m_pwf.chunkSize > 0){
                if(_client->available() < 2) { // avoid getting out of sync
                    AUDIO_INFO("webfile chunked: not enough bytes available for skipCRLF");
                    return;
                }
                int a =_client->read(); if(a != 0x0D) log_w("chunk count error, expected: 0x0D, received: 0x%02X", a); // skipCR
                int b =_client->read(); if(b != 0x0A) log_w("chunk count error, expected: 0x0A, received: 0x%02X", b); // skipLF
            }
            m_pwf.chunkSize = readChunkSize(&readedBytes);
            if(m_pwf.chunkSize == 0) m_f_allDataReceived = true; // last chunk
            // log_w("chunk size: %d", chunkSize);
            m_contentlength += m_pwf.chunkSize;
            m_audioDataSize += m_pwf.chunkSize;
        }
        availableBytes = min(availableBytes, m_contentlength - m_pwf.byteCounter);
    }
    if(!m_f_chunked && m_pwf.byteCounter >= m_audioDataSize) {m_f_allDataReceived = true;}
    if(!m_pwf.f_clientIsConnected) {if(!m_f_allDataReceived)  m_f_allDataReceived = true;} // connection closed
    // log_w("byteCounter %u >= m_audioDataSize %u, m_f_allDataReceived % i", byteCounter, m_contentlength, m_f_allDataReceived);

    // if the buffer is often almost empty issue a warning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_stream) {if(streamDetection(availableBytes)) return;}
    availableBytes = min(availableBytes, (uint32_t)InBuff.writeSpace());
    int32_t bytesAddedToBuffer = 0;
    if(m_pwf.f_clientIsConnected) bytesAddedToBuffer = _client->read(InBuff.getWritePtr(), availableBytes);
    if(bytesAddedToBuffer > 0) {
        m_webFilePos += bytesAddedToBuffer;
        m_pwf.byteCounter += bytesAddedToBuffer;
        if(m_f_chunked) m_chunkcount -= bytesAddedToBuffer;
        if(m_controlCounter == 100) m_pwf.audioDataCount += bytesAddedToBuffer;
        InBuff.bytesWritten(bytesAddedToBuffer);
    }

    // we have a webfile, read the file header first - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if(m_controlCounter != 100) {
        if(InBuff.bufferFilled() > m_pwf.maxFrameSize || (m_contentlength && (InBuff.bufferFilled() == m_contentlength))) { // at least one complete frame or the file is smaller
            int32_t bytesRead = readAudioHeader(InBuff.getMaxAvailableBytes());
            if(bytesRead > 0) InBuff.bytesWasRead(bytesRead);
        }
        return;
    }

    if(m_codec == CODEC_OGG) { // log_i("determine correct codec here");
        uint8_t codec = determineOggCodec(InBuff.getReadPtr(), m_pwf.maxFrameSize);
        if(codec == CODEC_VORBIS) {m_codec = codec; AUDIO_INFO("format is vorbis"); return;}
        else if(codec == CODEC_FLAC)   {m_codec = codec; AUDIO_INFO("format is flac"); return;}
        else if(codec == CODEC_OPUS)  {m_codec = codec; AUDIO_INFO("format is opus");
							AUDIO_ERROR("can't play OPUS yet"); /*stopSong();*/  return;}
        else {stopSong(); return;}
    }

    if(!m_f_stream && m_controlCounter == 100) {
        m_f_stream = true; // ready to play the audio data
        uint16_t filltime = millis() - m_t0;
        AUDIO_INFO("Webfile: stream ready, buffer filled in %d ms", filltime);
        return;
    }

    // end of webfile reached? - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_eof) { // m_f_eof and m_f_ID3v1TagFound will be set in playAudioData()
        if(m_f_ID3v1TagFound) readID3V1Tag();

        stopSong();
        if(m_f_tts) {
            AUDIO_INFO("End of speech \"%s\"", m_speechtxt.c_get());
            if(audio_eof_speech) audio_eof_speech(m_speechtxt.c_get());
        }
        else {
            AUDIO_INFO("End of webstream: \"%s\"", m_lastHost.c_get());
            if(audio_eof_stream) audio_eof_stream(m_lastHost.c_get());
        }
        return;
    }
    return;
}
//##################################################################
void Audio::processWebStreamTS() {
    uint32_t        availableBytes;     // available bytes in stream
    uint8_t         ts_packetStart = 0;
    uint8_t         ts_packetLength = 0;

    // first call, set some values to default ———————————————————————————————————
    if(m_f_firstCall) {   // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        m_f_m3u8data = true;
        m_pwsst.f_firstPacket = true;
        m_pwsst.f_chunkFinished = false;
        m_pwsst.f_nextRound = false;
        m_pwsst.byteCounter = 0;
        m_pwsst.chunkSize = 0;
        m_t0 = millis();
        m_pwsst.ts_packetPtr = 0;
        if(!m_pwsst.ts_packet.valid()) m_pwsst.ts_packet.alloc_array(m_pwsst.ts_packetsize, "TS Packet"); // first init
    } //—————————————————————————————————————————————————————————————————————————

    if(m_dataMode != AUDIO_DATA) return; // guard

nextRound:
    availableBytes = _client->available();
    if(availableBytes) {
        /* If the m3u8 stream uses 'chunked data transfer' no content length is supplied. Then the chunk size determines the audio data to be processed.
           However, the chunk size in some streams is limited to 32768 bytes, although the chunk can be larger. Then the chunk size is
           calculated again. The data used to calculate (here readedBytes) the chunk size is not part of it.
        */
        uint8_t readedBytes = 0;
        uint32_t minAvBytes = 0;
        if(m_f_chunked && m_pwsst.chunkSize == m_pwsst.byteCounter) {m_pwsst.chunkSize += readChunkSize(&readedBytes); goto exit;}
        if(m_pwsst.chunkSize) minAvBytes = min3(availableBytes, m_pwsst.ts_packetsize - m_pwsst.ts_packetPtr, m_pwsst.chunkSize - m_pwsst.byteCounter);
        else          minAvBytes = min(availableBytes, (uint32_t)(m_pwsst.ts_packetsize - m_pwsst.ts_packetPtr));

        int res = _client->read(m_pwsst.ts_packet.get() + m_pwsst.ts_packetPtr, minAvBytes);
        if(res > 0) {
            m_pwsst.ts_packetPtr += res;
            m_pwsst.byteCounter += res;
            if(m_pwsst.ts_packetPtr < m_pwsst.ts_packetsize) return; // not enough data yet, the process must be repeated if the packet size (188 bytes) is not reached
            m_pwsst.ts_packetPtr = 0;
            if(m_pwsst.f_firstPacket) { // search for ID3 Header in the first packet
                m_pwsst.f_firstPacket = false;
                uint8_t ID3_HeaderSize = process_m3u8_ID3_Header(m_pwsst.ts_packet.get());
                if(ID3_HeaderSize > m_pwsst.ts_packetsize) {
                    AUDIO_ERROR("ID3 Header is too big");
                    stopSong();
                    return;
                }
                if(ID3_HeaderSize) {
                    memcpy(m_pwsst.ts_packet.get(), &m_pwsst.ts_packet.get()[ID3_HeaderSize], m_pwsst.ts_packetsize - ID3_HeaderSize);
                    m_pwsst.ts_packetPtr = m_pwsst.ts_packetsize - ID3_HeaderSize;
                    return;
                }
            }
            ts_parsePacket(&m_pwsst.ts_packet.get()[0], &ts_packetStart, &ts_packetLength); // todo: check for errors

            if(ts_packetLength) {
                size_t ws = InBuff.writeSpace();
                if(ws >= ts_packetLength) {
                    memcpy(InBuff.getWritePtr(), m_pwsst.ts_packet.get() + ts_packetStart, ts_packetLength);
                    InBuff.bytesWritten(ts_packetLength);
                    m_pwsst.f_nextRound = true;
                }
                else {
                    memcpy(InBuff.getWritePtr(), m_pwsst.ts_packet.get() + ts_packetStart, ws);
                    InBuff.bytesWritten(ws);
                    memcpy(InBuff.getWritePtr(), &m_pwsst.ts_packet.get()[ws + ts_packetStart], ts_packetLength - ws);
                    InBuff.bytesWritten(ts_packetLength - ws);
                }
            }
            if (m_pwsst.byteCounter == m_contentlength || m_pwsst.byteCounter == m_pwsst.chunkSize) {
                m_pwsst.f_chunkFinished = true;
                m_pwsst.f_nextRound = false;
                m_pwsst.byteCounter = 0;
                int av = _client->available();
                if(av == 7) for(int i = 0; i < av; i++) _client->read(); // waste last chunksize: 0x0D 0x0A 0x30 0x0D 0x0A 0x0D 0x0A (==0, end of chunked data transfer)
            }
            if(m_contentlength && m_pwsst.byteCounter > m_contentlength) {AUDIO_ERROR("byteCounter overflow, byteCounter: %d, contentlength: %d", m_pwsst.byteCounter, m_contentlength); return;}
            if(m_pwsst.chunkSize       && m_pwsst.byteCounter > m_pwsst.chunkSize)       {AUDIO_ERROR("byteCounter overflow, byteCounter: %d, chunkSize: %d", m_pwsst.byteCounter, m_pwsst.chunkSize); return;}
        }
    }
    if(m_pwsst.f_chunkFinished) {
        if(InBuff.bufferFilled() < 60000) {
            m_pwsst.f_chunkFinished = false;
            m_f_continue = true;
        }
    }

    // if the buffer is often almost empty issue a warning - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_stream) {
        if(streamDetection(availableBytes)) goto exit;
    }

    // buffer fill routine  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(true) {                                                  // statement has no effect
        if(InBuff.bufferFilled() > 60000 && !m_f_stream) {     // waiting for buffer filled
            m_f_stream = true;                                  // ready to play the audio data
            uint16_t filltime = millis() - m_t0;
            AUDIO_INFO("stream ready");
            AUDIO_INFO("buffer filled in %d ms", filltime);
        }
    }
    if(m_pwsst.f_nextRound){goto nextRound;}
exit:
    return;
}
//##################################################################
void Audio::processWebStreamHLS() {

    // first call, set some values to default - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(m_f_firstCall) { // runs only ont time per connection, prepare for start
        m_f_firstCall = false;
        m_f_m3u8data = true;
        m_t0 = millis();
        m_controlCounter = 0;

        m_pwsHLS.maxFrameSize = InBuff.getMaxBlockSize(); // every mp3/aac frame is not bigger
        m_pwsHLS.ID3BuffSize = 4096;
        m_pwsHLS.availableBytes = 0;
        m_pwsHLS.firstBytes = true;
        m_pwsHLS.f_chunkFinished = false;
        m_pwsHLS.byteCounter = 0;
        m_pwsHLS.chunkSize = 0;
        m_pwsHLS.ID3WritePtr = 0;
        m_pwsHLS.ID3ReadPtr = 0;
        m_pwsHLS.ID3Buff.alloc(m_pwsHLS.ID3BuffSize, "ID3Buff");
    }

    if(m_dataMode != AUDIO_DATA) return; // guard

    m_pwsHLS.availableBytes = _client->available();
    if(m_pwsHLS.availableBytes) { // an ID3 header could come here
        uint8_t readedBytes = 0;

        if(m_f_chunked && !m_pwsHLS.chunkSize) {
            m_pwsHLS.chunkSize = readChunkSize(&readedBytes);
            m_pwsHLS.byteCounter += readedBytes;
        }

        if(m_pwsHLS.firstBytes) {
            if(m_pwsHLS.ID3WritePtr < m_pwsHLS.ID3BuffSize) {
                m_pwsHLS.ID3WritePtr += _client->readBytes(&m_pwsHLS.ID3Buff[m_pwsHLS.ID3WritePtr], m_pwsHLS.ID3BuffSize - m_pwsHLS.ID3WritePtr);
                return;
            }
            if(m_controlCounter < 100) {
                int res = read_ID3_Header(&m_pwsHLS.ID3Buff[m_pwsHLS.ID3ReadPtr], m_pwsHLS.ID3BuffSize - m_pwsHLS.ID3ReadPtr);
                if(res >= 0) m_pwsHLS.ID3ReadPtr += res;
                if(m_pwsHLS.ID3ReadPtr > m_pwsHLS.ID3BuffSize) {
                    AUDIO_ERROR("buffer overflow");
                    stopSong();
                    return;
                }
                return;
            }
            if(m_controlCounter != 100) return;

            size_t ws = InBuff.writeSpace();
            if(ws >= m_pwsHLS.ID3BuffSize - m_pwsHLS.ID3ReadPtr) {
                memcpy(InBuff.getWritePtr(), &m_pwsHLS.ID3Buff[m_pwsHLS.ID3ReadPtr], m_pwsHLS.ID3BuffSize - m_pwsHLS.ID3ReadPtr);
                InBuff.bytesWritten(m_pwsHLS.ID3BuffSize - m_pwsHLS.ID3ReadPtr);
            }
            else {
                memcpy(InBuff.getWritePtr(), &m_pwsHLS.ID3Buff[m_pwsHLS.ID3ReadPtr], ws);
                InBuff.bytesWritten(ws);
                memcpy(InBuff.getWritePtr(), &m_pwsHLS.ID3Buff[ws + m_pwsHLS.ID3ReadPtr], m_pwsHLS.ID3BuffSize - (m_pwsHLS.ID3ReadPtr + ws));
                InBuff.bytesWritten(m_pwsHLS.ID3BuffSize - (m_pwsHLS.ID3ReadPtr + ws));
            }
            m_pwsHLS.ID3Buff.reset();
            m_pwsHLS.byteCounter += m_pwsHLS.ID3BuffSize;
            m_pwsHLS.firstBytes = false;
        }

        size_t bytesWasWritten = 0;
        if(InBuff.writeSpace() >= m_pwsHLS.availableBytes) {
        //    if(availableBytes > 1024) availableBytes = 1024; // 1K throttle
            bytesWasWritten = _client->read(InBuff.getWritePtr(), m_pwsHLS.availableBytes);
        }
        else { bytesWasWritten = _client->read(InBuff.getWritePtr(), InBuff.writeSpace()); }
        InBuff.bytesWritten(bytesWasWritten);

        m_pwsHLS.byteCounter += bytesWasWritten;

        if(m_pwsHLS.byteCounter == m_contentlength || m_pwsHLS.byteCounter == m_pwsHLS.chunkSize) {
            m_pwsHLS.f_chunkFinished = true;
            m_pwsHLS.byteCounter = 0;
        }
    }

    if(m_pwsHLS.f_chunkFinished) {
        if(InBuff.bufferFilled() < 50000) {
            m_pwsHLS.f_chunkFinished = false;
            m_f_continue = true;
        }
    }

    // if the buffer is often almost empty issue a warning or try a new connection - - - - - - - - - - - - - - - - - - -
    if(m_f_stream) {
        if(streamDetection(m_pwsHLS.availableBytes)) return;
    }

    if(InBuff.bufferFilled() > m_pwsHLS.maxFrameSize && !m_f_stream) { // waiting for buffer filled
        m_f_stream = true;                                    // ready to play the audio data
        //uint16_t filltime = millis() - m_t0;
        AUDIO_INFO("stream ready");
        // AUDIO_INFO("buffer filled in %u ms", filltime);
    }
    return;
}
//##################################################################
void Audio::playAudioData() {

    if(!m_f_stream || m_f_eof || m_f_lockInBuffer || !m_f_running){return;} // guard, stream not ready or eof reached or InBuff is locked or not running
    if(m_dataMode == AUDIO_LOCALFILE && m_resumeFilePos != -1){    return;} // guard, m_resumeFilePos is set (-1 is default)
//    if(m_validSamples) {playChunk();                               return;} // guard, play samples first
    //--------------------------------------------------------------------------------
    m_pad.count = 0;
    m_pad.bytesToDecode = InBuff.bufferFilled();
    m_pad.bytesDecoded = 0;

    if(m_f_firstPlayCall) {
        m_f_firstPlayCall = false;
        m_pad.count = 0;
        m_pad.oldAudioDataSize = 0;
        m_sumBytesDecoded = 0;
        m_bytesNotDecoded = 0;
        m_pad.lastFrames = false;
        m_f_eof = false;
    }
    //--------------------------------------------------------------------------------

    m_f_audioTaskIsDecoding = true;

    if((m_dataMode == AUDIO_LOCALFILE || m_streamType == ST_WEBFILE) && m_playlistFormat != FORMAT_M3U8)  { // local file or webfile but not m3u8 file
        if(!m_audioDataSize) goto exit; // no data to decode if filesize is 0
        if(m_audioDataSize != m_pad.oldAudioDataSize) { // Special case: Metadata in ogg files are recognized by the decoder,
            if(m_f_ogg)m_sumBytesDecoded = 0;     // after which m_audioDataStart and m_audioDataSize are updated.
            if(m_f_ogg)m_bytesNotDecoded = 0;
            m_pad.oldAudioDataSize = m_audioDataSize;
        }

        m_pad.bytesToDecode = m_audioDataSize - m_sumBytesDecoded;

        if(m_pad.bytesToDecode < InBuff.getMaxBlockSize() * 2 && m_f_allDataReceived) { // last frames to decode
            m_pad.lastFrames = true;
        }
        if(m_sumBytesDecoded > 0){
            if(m_sumBytesDecoded >= m_audioDataSize) {m_f_eof = true; goto exit;} // file end reached
            if(m_pad.bytesToDecode <= 0)                   {m_f_eof = true; goto exit;} // file end reached
        }
    }
    else{if(InBuff.bufferFilled() < InBuff.getMaxBlockSize() && m_f_allDataReceived) {m_pad.lastFrames = true;}}

    m_pad.bytesToDecode = min((int32_t)InBuff.getMaxBlockSize(), m_pad.bytesToDecode);

    if(m_pad.lastFrames){
        m_pad.bytesDecoded = sendBytes(InBuff.getReadPtr(), m_pad.bytesToDecode);
    }
    else{
        if(InBuff.bufferFilled() >= InBuff.getMaxBlockSize()) m_pad.bytesDecoded = sendBytes(InBuff.getReadPtr(), m_pad.bytesToDecode);
        else m_pad.bytesDecoded = 0; // Inbuff not filled enough
    }

    if(m_pad.bytesDecoded <= 0) {
        if(m_pad.lastFrames) {m_f_eof = true; goto exit;} // end of file reached
        m_pad.count++;
        vTaskDelay(20); // wait for data
        if(m_pad.count == 30) {if(m_f_allDataReceived) m_f_eof = true;}  // maybe slow stream
        goto exit; // syncword at pos0
    }
    m_pad.count = 0;

    if(m_pad.bytesDecoded > 0) {
        InBuff.bytesWasRead(m_pad.bytesDecoded);
        if(m_controlCounter == 100) m_sumBytesDecoded += m_pad.bytesDecoded;
        // log_w("m_audioDataSize: %d, m_sumBytesDecoded: %d, diff: %d", m_audioDataSize, m_sumBytesDecoded, m_audioDataSize - m_sumBytesDecoded);
        if(m_codec == CODEC_MP3 && m_audioDataSize - m_sumBytesDecoded == 128){
            m_f_ID3v1TagFound = true; m_f_eof = true;
        }
        if(m_f_allDataReceived && InBuff.bufferFilled() == 0) {
            m_f_eof = true; // end of file reached
        }
    }

exit:
    m_f_audioTaskIsDecoding = false;
    return;
}
//##################################################################
bool Audio::parseHttpResponseHeader() { // this is the response to a GET / request

    if(m_dataMode != HTTP_RESPONSE_HEADER) return false;
    if(!m_currentHost.valid()) {AUDIO_ERROR("m_currentHost is empty"); return false;}

    m_phrh.ctime = millis();
    m_phrh.timeout = 4500; // ms

    if(_client->available() == 0) {
        if(!m_phrh.f_time) {
            m_phrh.stime = millis();
            m_phrh.f_time = true;
        }
        if((millis() - m_phrh.stime) > m_phrh.timeout) {
            AUDIO_ERROR("timeout");
            m_phrh.f_time = false;
            return false;
        }
    }
    m_phrh.f_time = false;

    ps_ptr<char>rhl;
    rhl.alloc(1024, "rhl"); // responseHeaderline
    rhl.clear();
    bool ct_seen = false;

    while(true) { // outer while
        uint16_t pos = 0;
        if((millis() - m_phrh.ctime) > m_phrh.timeout) {
            AUDIO_ERROR("timeout");
            m_f_timeout = true;
            goto exit;
        }
        while(_client->available()) {
            uint8_t b = _client->read();
            if(b == '\n') {
                if(!pos) { // empty line received, is the last line of this responseHeader
                    if(ct_seen) goto lastToDo;
                    else {if(!_client->available())  goto exit;}
                }
                break;
            }
            if(b == '\r') rhl[pos] = 0;
            if(b < 0x20) continue;
            rhl[pos] = b;
            pos++;
            if(pos == 1023) {
                pos = 1022;
                continue;
            }
            if(pos == 1022) {
                rhl[pos] = '\0';
                log_w("responseHeaderline overflow");
            }
        } // inner while
        if(!pos) {
            vTaskDelay(5);
            continue;
        }
        if(rhl.starts_with_icase("HTTP/")) { // HTTP status error code
            char statusCode[5];
            statusCode[0] = rhl[9];
            statusCode[1] = rhl[10];
            statusCode[2] = rhl[11];
            statusCode[3] = '\0';
            int sc = atoi(statusCode);
            if(sc > 310) { // e.g. HTTP/1.1 301 Moved Permanently
                AUDIO_ERROR(rhl.get());
                goto exit;
            }
        }

        else if(rhl.starts_with_icase("content-type:")) { // content-type: text/html; charset=UTF-8
            int idx = rhl.index_of(';', 13);
            if(idx > 0) rhl[idx] = '\0';
            if(parseContentType(rhl.get() + 13)) ct_seen = true;
            else{
                log_w("unknown contentType %s", rhl.get() + 13);
                goto exit;
            }
        }

        else if(rhl.starts_with_icase("location:")) {
            int pos = rhl.index_of_icase("http", 0);
            if(pos >= 0) {
                const char* c_host = (rhl.get() + pos);
                if(!m_currentHost.equals(c_host)) { // prevent a loop
                    int pos_slash = indexOf(c_host, "/", 9);
                    if(pos_slash > 9) {
                        if(!strncmp(c_host, m_currentHost.get(), pos_slash)) {
                            AUDIO_INFO("redirect to new extension at existing host \"%s\"", c_host);
                            if(m_playlistFormat == FORMAT_M3U8) {
                                m_lastHost.assign(c_host);
                                m_f_m3u8data = true;
                            }
                            httpPrint(c_host);
                            while(_client->available()) _client->read(); // empty client buffer
                            return true;
                        }
                    }
                    AUDIO_INFO("redirect to new host \"%s\"", c_host);
                    m_f_reset_m3u8Codec = false;
                    httpPrint(c_host);
                    return true;
                }
            }
            else{
                log_w("unknown redirection: %s", rhl.c_get());
            }
        }

        else if(rhl.starts_with_icase("content-encoding:")) {
            AUDIO_INFO("%s", rhl.get());
            if(rhl.contains("gzip")) {
                AUDIO_INFO("can't extract gzip");
                goto exit;
            }
        }

        else if(rhl.starts_with_icase("content-disposition:")) { // e.g we have this headerline:  content-disposition: attachment; filename=stream.asx
            int idx = rhl.index_of_icase("filename=");
            if (idx >= 0)  {
                ps_ptr<char> fn;
                fn.assign(rhl.get() + idx + 9); // Position directly after "filename="
                fn.replace("\"", ""); // remove '\"' around filename if present
                AUDIO_INFO("Filename is %s", fn.get());
            }
        }
        else if(rhl.starts_with_icase("connection:")) {
            if(rhl.contains_with_icase("close")) {m_f_connectionClose = true; /* AUDIO_ERROR("connection will be closed"); */} // ends after ogg last Page is set
        }

        else if(rhl.starts_with_icase("icy-genre:")) {
            AUDIO_INFO("icy-genre: %s", rhl.get() + 10 ); // Ambient, Rock, etc
        }

        else if(rhl.starts_with_icase("icy-logo:")) {
            ps_ptr<char> icyLogo;
            icyLogo.assign(rhl.get() + 9); // Get logo URL
            icyLogo.trim();
            if(icyLogo.strlen() > 0){
                AUDIO_INFO("icy-logo: %s", icyLogo.get());
                if(audio_icylogo) audio_icylogo(icyLogo.get());
            }
        }

        else if(rhl.starts_with_icase("icy-br:")) {
            ps_ptr<char>c_bitRate; c_bitRate.assign(rhl.get() + 7);
            c_bitRate.trim();
            c_bitRate.append("000"); // Found bitrate tag, read the bitrate in Kbit
            setBitrate(c_bitRate.to_uint64());
            AUDIO_INFO("icy-bitrate: %s", c_bitRate.get());
            if(audio_bitrate) audio_bitrate(c_bitRate.get());
        }

        else if(rhl.starts_with_icase("icy-metaint:")) {
            const char* c_metaint = (rhl.get() + 12);
            int32_t     i_metaint = atoi(c_metaint);
            m_metaint = i_metaint;
            if(m_metaint) m_f_metadata = true; // Multimediastream
        }

        else if(rhl.starts_with_icase("icy-name:")) {
        // log_w("%s", rhl.get());
            ps_ptr<char> icyName;
            icyName.assign(rhl.get() + 9); // Get station name
            icyName.trim();
            if(icyName.strlen() > 0) {
                AUDIO_INFO("icy-name: %s", icyName.get());
                if(audio_showstation) audio_showstation(icyName.get());
            }
        }

        else if(rhl.starts_with_icase("content-length:")) {
            const char* c_cl = (rhl.get() + 15);
            int32_t     i_cl = atoi(c_cl);
            m_contentlength = i_cl;
            m_streamType = ST_WEBFILE; // Stream comes from a fileserver
            // AUDIO_INFO("content-length: %lu", (long unsigned int)m_contentlength);
        }

        else if(rhl.starts_with_icase("icy-description:")) {
            const char* c_idesc = (rhl.get() + 16);
            while(c_idesc[0] == ' ') c_idesc++;
            latinToUTF8(rhl); // if already UTF-8 do nothing, otherwise convert to UTF-8
            if(strlen(c_idesc) > 0 && specialIndexOf((uint8_t*)c_idesc, "24bit", 0) > 0) {
                AUDIO_INFO("icy-description: %s has to be 8 or 16", c_idesc);
                stopSong();
            }
            if(audio_icydescription) audio_icydescription(c_idesc);
        }

        else if(rhl.starts_with_icase("transfer-encoding:")) {
            if(rhl.ends_with_icase("chunked")) { // Station provides chunked transfer
                m_f_chunked = true;
                AUDIO_INFO("chunked data transfer");
                m_chunkcount = 0; // Expect chunkcount in DATA
            }
        }

        else if(rhl.starts_with("accept-ranges:")) {
            if(rhl.ends_with_icase("bytes")) m_f_acceptRanges = true;
        //    log_w("%s", rhl);
        }

        else if(rhl.starts_with("content-range:")) {
        //    log_w("%s", rhl);
        }

        else if(rhl.starts_with_icase("icy-url:")) {
            char* icyurl = (rhl.get() + 8);
            trim(icyurl);
            if(audio_icyurl) audio_icyurl(icyurl);
        }

        else if(rhl.starts_with_icase("www-authenticate:")) {
            AUDIO_INFO("authentification failed, wrong credentials?");
            goto exit;
        }
        else { ; }
    } // outer while

exit: // termination condition
    if(audio_showstation) audio_showstation("");
    if(audio_icydescription) audio_icydescription("");
    if(audio_icyurl) audio_icyurl("");
    m_dataMode = AUDIO_NONE;
    stopSong();
    return false;

lastToDo:
    m_streamType = ST_WEBSTREAM;
    if(m_contentlength > 0)          m_streamType = ST_WEBFILE;
    if(m_f_chunked)                  m_streamType = ST_WEBFILE; // Stream comes from a fileserver, metadata have webstreams
    if(m_f_chunked && m_f_metadata)  m_streamType = ST_WEBSTREAM;


    if(m_codec != CODEC_NONE) {
        m_dataMode = AUDIO_DATA; // Expecting data now

        if(m_bitRate == 0){
           if(m_avr_bitrate != 0) m_bitRate = m_avr_bitrate;
           else m_bitRate = (wram_read(0x1e05) & 0xFF) * 1000; // Read average bitRate from WRAM register
                sprintf(m_chbuf, "%lu", m_bitRate);
                audio_bitrate(m_chbuf);
        }
    }
    else if(m_playlistFormat != FORMAT_NONE) {
        m_dataMode = AUDIO_PLAYLISTINIT; // playlist expected
        // log_i("now parse playlist");
    }
    else {
        AUDIO_ERROR("unknown content found at: %s", m_currentHost.c_get());
        goto exit;
    }
    return true;
}
//##################################################################
bool Audio::parseContentType(char* ct) {
    enum : int { CT_NONE, CT_MP3, CT_AAC, CT_M4A, CT_WAV, CT_FLAC, CT_PLS, CT_M3U, CT_ASX, CT_M3U8, CT_TXT, CT_AACP, CT_OPUS, CT_OGG, CT_VORBIS };

    strlower(ct);
    trim(ct);

    m_codec = CODEC_NONE;
    int ct_val = CT_NONE;

    if(!strcmp(ct, "audio/mpeg")) ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/mpeg3"))                   ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/x-mpeg"))                  ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/x-mpeg-3"))                ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/mp3"))                     ct_val = CT_MP3;
    else if(!strcmp(ct, "audio/aac"))                     ct_val = CT_AAC;
    else if(!strcmp(ct, "audio/x-aac"))                   ct_val = CT_AAC;
    else if(!strcmp(ct, "audio/aacp"))                    ct_val = CT_AAC;
    else if(!strcmp(ct, "video/mp2t")){                   ct_val = CT_AAC;  if(m_m3u8Codec == CODEC_MP3) ct_val = CT_MP3;} // see m3u8redirection()
    else if(!strcmp(ct, "audio/mp4"))                     ct_val = CT_M4A;
    else if(!strcmp(ct, "audio/m4a"))                     ct_val = CT_M4A;
    else if(!strcmp(ct, "audio/x-m4a"))                   ct_val = CT_M4A;
    else if(!strcmp(ct, "audio/wav"))                     ct_val = CT_WAV;
    else if(!strcmp(ct, "audio/x-wav"))                   ct_val = CT_WAV;
    else if(!strcmp(ct, "audio/flac"))                    ct_val = CT_FLAC;
    else if(!strcmp(ct, "audio/x-flac"))                  ct_val = CT_FLAC;
    else if(!strcmp(ct, "audio/scpls"))                   ct_val = CT_PLS;
    else if(!strcmp(ct, "audio/x-scpls"))                 ct_val = CT_PLS;
    else if(!strcmp(ct, "application/pls+xml"))           ct_val = CT_PLS;
    else if(!strcmp(ct, "audio/mpegurl")) {               ct_val = CT_M3U;  if(m_expectedPlsFmt == FORMAT_M3U8) ct_val = CT_M3U8;}
    else if(!strcmp(ct, "audio/x-mpegurl"))               ct_val = CT_M3U;
    else if(!strcmp(ct, "audio/ms-asf"))                  ct_val = CT_ASX;
    else if(!strcmp(ct, "video/x-ms-asf"))                ct_val = CT_ASX;
    else if(!strcmp(ct, "audio/x-ms-asx"))                ct_val = CT_ASX;  // #413

    else if(!strcmp(ct, "application/ogg"))               ct_val = CT_OGG;
    else if(!strcmp(ct, "audio/ogg"))                     ct_val = CT_OGG;

    else if(!strcmp(ct, "application/vorbis"))  ct_val = CT_VORBIS;
    else if(!strcmp(ct, "audio/vorbis"))        ct_val = CT_VORBIS;

    else if(!strcmp(ct, "audio/flac"))       ct_val = CT_FLAC;
    else if(!strcmp(ct, "audio/x-flac"))     ct_val = CT_FLAC;

    else if(!strcmp(ct, "application/opus"))  ct_val = CT_OPUS;
    else if(!strcmp(ct, "audio/opus"))        ct_val = CT_OPUS;

    else if(!strcmp(ct, "application/vnd.apple.mpegurl")) ct_val = CT_M3U8;
    else if(!strcmp(ct, "application/x-mpegurl"))         ct_val = CT_M3U8;

    else if(!strcmp(ct, "application/octet-stream"))      ct_val = CT_TXT;  // ??? listen.radionomy.com/1oldies before redirection
    else if(!strcmp(ct, "text/html"))                     ct_val = CT_TXT;
    else if(!strcmp(ct, "text/plain"))                    ct_val = CT_TXT;

    else if(ct_val == CT_NONE) {
        AUDIO_ERROR("ContentType %s not supported", ct);
        return false; // nothing valid had been seen
    }
    else { ; }
    switch(ct_val) {
        case CT_MP3:
            m_codec = CODEC_MP3;
            // { log_i("ContentType %s, format is mp3", ct); } // ok is likely mp3
            audio_info("format is mp3");
            break;
        case CT_AAC:
            m_codec = CODEC_AAC;
            // { log_i("ContentType %s, format is aac", ct); }
            audio_info("format is aac");
            break;
        case CT_M4A:
            m_codec = CODEC_M4A;
            // { log_i("ContentType %s, format is aac", ct); }
            audio_info("format is aac");
            break;
        case CT_FLAC:
            m_codec = CODEC_FLAC;
            // { log_i("ContentType %s, format is flac", ct); }
            audio_info("format is flac");
            break;
        case CT_OPUS:
            m_codec = CODEC_OPUS;
            m_f_ogg = true; // opus is ogg
            // { log_i("ContentType %s, format is opus", ct); }
            audio_info("format is opus");
            break;
        case CT_VORBIS:
            m_codec = CODEC_VORBIS;
            m_f_ogg = true; // vorbis is ogg
            // { log_i("ContentType %s, format is vorbis", ct); }
            audio_info("format is vorbis");
            break;
        case CT_WAV:
            m_codec = CODEC_WAV;
            // { log_i("ContentType %s, format is wav", ct); }
            audio_info("format is wav");
            break;
        case CT_OGG:
            m_codec = CODEC_OGG; audio_info("format is ogg"); 	 // determine in first OGG packet -OPUS, VORBIS, FLAC
            m_f_ogg = true; // maybe flac or opus or vorbis
            break;
        case CT_PLS: m_playlistFormat = FORMAT_PLS; break;
        case CT_M3U: m_playlistFormat = FORMAT_M3U; break;
        case CT_ASX: m_playlistFormat = FORMAT_ASX; break;
        case CT_M3U8: m_playlistFormat = FORMAT_M3U8; break;
        case CT_TXT: // overwrite text/plain
            if(m_expectedCodec == CODEC_AAC) {
                m_codec = CODEC_AAC; audio_info("format is aac"); /* log_i("set ct from M3U8 to AAC");*/ }
            if(m_expectedCodec == CODEC_MP3) {
                m_codec = CODEC_MP3; audio_info("format is mp3"); /* log_i("set ct from M3U8 to MP3");*/ }
            if(m_expectedPlsFmt == FORMAT_ASX) {
                m_playlistFormat = FORMAT_ASX;  /* log_i("set playlist format to ASX");*/ }
            if(m_expectedPlsFmt == FORMAT_M3U) {
                m_playlistFormat = FORMAT_M3U;  /* log_i("set playlist format to M3U");*/ }
            if(m_expectedPlsFmt == FORMAT_M3U8) {
                m_playlistFormat = FORMAT_M3U8; /* log_i("set playlist format to M3U8");*/ }
            if(m_expectedPlsFmt == FORMAT_PLS) {
                m_playlistFormat = FORMAT_PLS; /* log_i("set playlist format to PLS");*/ }
            break;
        default:
            AUDIO_ERROR("%s, unsupported audio format", ct);
            return false;
            break;
    }
    return true;
}
// clang-format on
//##################################################################
void Audio::showstreamtitle(char* ml) {
    // example for ml:
    // StreamTitle='Oliver Frank - Mega Hitmix';StreamUrl='www.radio-welle-woerthersee.at';
    // or adw_ad='true';durationMilliseconds='10135';adId='34254';insertionType='preroll';
    // html: 'Bielszy odcie&#324; bluesa 682 cz.1' --> 'Bielszy odcień bluesa 682 cz.1'

    if(!ml) return;
    ps_ptr<char>title = {};
    ps_ptr<char>artist = {};
    ps_ptr<char>streamTitle = {};
    ps_ptr<char>sUrl = {};

    htmlToUTF8(ml); // convert to UTF-8

    int16_t  idx1 = 0, idx2, idx4, idx5, idx6, idx7, titleLen = 0, artistLen = 0, titleStart = 0, artistStart = 0;

    // if(idx1 < 0) idx1 = indexOf(ml, "Title:", 0); // Title found (e.g. https://stream-hls.bauermedia.pt/comercial.aac/playlist.m3u8)
    // if(idx1 < 0) idx1 = indexOf(ml, "title:", 0); // Title found (e.g. #EXTINF:10,title="The Dan Patrick Show (M-F 9a-12p ET)",artist="zc1401"

    if(indexOf(ml, "StreamTitle='<?xml version=", 0) == 0) {
        /* e.g. xmlStreamTitle
              StreamTitle='<?xml version="1.0" encoding="utf-8"?><RadioInfo><Table><DB_ALBUM_ID>37364</DB_ALBUM_ID>
              <DB_ALBUM_IMAGE>00000037364.jpg</DB_ALBUM_IMAGE><DB_ALBUM_NAME>Boyfriend</DB_ALBUM_NAME>
              <DB_ALBUM_TYPE>Single</DB_ALBUM_TYPE><DB_DALET_ARTIST_NAME>DOVE CAMERON</DB_DALET_ARTIST_NAME>
              <DB_DALET_ITEM_CODE>CD4161</DB_DALET_ITEM_CODE><DB_DALET_TITLE_NAME>BOYFRIEND</DB_DALET_TITLE_NAME>
              <DB_FK_SITE_ID>2</DB_FK_SITE_ID><DB_IS_MUSIC>1</DB_IS_MUSIC><DB_LEAD_ARTIST_ID>26303</DB_LEAD_ARTIST_ID>
              <DB_LEAD_ARTIST_NAME>Dove Cameron</DB_LEAD_ARTIST_NAME><DB_RADIO_IMAGE>cidadefm.jpg</DB_RADIO_IMAGE>
              <DB_RADIO_NAME>Cidade</DB_RADIO_NAME><DB_SONG_ID>120126</DB_SONG_ID><DB_SONG_LYRIC>60981</DB_SONG_LYRIC>
              <DB_SONG_NAME>Boyfriend</DB_SONG_NAME></Table><AnimadorInfo><TITLE>Cidade</TITLE>
              <START_TIME_UTC>2022-11-15T22:00:00+00:00</START_TIME_UTC><END_TIME_UTC>2022-11-16T06:59:59+00:00
              </END_TIME_UTC><SHOW_NAME>Cidade</SHOW_NAME><SHOW_HOURS>22h às 07h</SHOW_HOURS><SHOW_PANEL>0</SHOW_PANEL>
              </AnimadorInfo></RadioInfo>';StreamUrl='';
        */
        idx4 = indexOf(ml, "<DB_DALET_TITLE_NAME>");
        idx5 = indexOf(ml, "</DB_DALET_TITLE_NAME>");
        idx6 = indexOf(ml, "<DB_LEAD_ARTIST_NAME>");
        idx7 = indexOf(ml, "</DB_LEAD_ARTIST_NAME>");
        if(idx4 == -1 || idx5 == -1) return;
        titleStart = idx4 + 21; // <DB_DALET_TITLE_NAME>
        titleLen = idx5 - titleStart;

        if(idx6 != -1 && idx7 != -1) {
            artistStart = idx6 + 21; // <DB_LEAD_ARTIST_NAME>
            artistLen = idx7 - artistStart;
        }
        if(titleLen) title.assign(ml + titleStart, titleLen);
        if(artistLen) artist.assign(ml + artistStart, artistLen);

        if(title.valid() && artist.valid()){
            streamTitle.assign(title.get());
            streamTitle.append(" - ");
            streamTitle.append(artist.get());
        }
        else if(title.valid()){
            streamTitle.assign(title.get());
        }
        else if(artist.valid()){
            streamTitle.assign(artist.get());
        }
    }

    else if(indexOf(ml, "StreamTitle='") == 0){
        titleStart = 13;
        idx2 = indexOf(ml, ";", 12);
        if(idx2 > titleStart + 1){
            titleLen = idx2 - 1 - titleStart;
            streamTitle.assign(ml + 13, titleLen);
        }
    }

    else if(startsWith(ml, "#EXTINF")){
        // extraxt StreamTitle from m3u #EXTINF line to icy-format
        // orig: #EXTINF:10,title="text="TitleName",artist="ArtistName"
        // conv: StreamTitle=TitleName - ArtistName
        // orig: #EXTINF:10,title="text=\"Spot Block End\" amgTrackId=\"9876543\"",artist=" ",url="length=\"00:00:00\""
        // conv: StreamTitle=text=\"Spot Block End\" amgTrackId=\"9876543\" -

        idx1 = indexOf(ml, "title=\"");
        idx2 = indexOf(ml, "artist=\"");
        if(idx1 > 0){
            int titleStart = idx1 + 7;
            int idx3 = indexOf(ml, "\"", titleStart);
            if(idx3 > titleStart){
                int titleLength = idx3 - titleStart;
                title.assign(ml + titleStart, titleLength);
                 if(title.starts_with("text=\\")){ // #EXTINF:10,title="text=\"Spot Block End\"",artist=" ",
                    titleStart += 7;
                    idx3 = indexOf(ml, "\\", titleStart);
                    if(idx3 > titleStart){
                        int titleLength = idx3 - titleStart;
                        title.assign(ml + titleStart, titleLength);
                    }
                }
            }
        }
        if(idx2 > 0){
            int artistStart = idx2 + 8;
            int idx3 = indexOf(ml, "\"", artistStart);
            if(idx3 > artistStart){
                int artistLength = idx3 - artistStart;
                artist.assign(ml + artistStart, artistLength);
                if(strcmp(artist.get(), " ") == 0) artist.reset();
            }
        }
        if(title.valid() && artist.valid()){
            streamTitle.assign(title.get());
            streamTitle.append(" - ");
            streamTitle.append(artist.get());
        }
        else if(title.valid()){
            streamTitle.assign(title.get());
        }
        else if(artist.valid()){
            streamTitle.assign(artist.get());
        }
    }
    else{
        ;
    }

    if(indexOf(ml, "StreamUrl=", 0) > 0){
        idx1 = indexOf(ml, "StreamUrl=", 0);
        idx2 = indexOf(ml, ";", idx1);
        if(idx1 >= 0 && idx2 > idx1) { // StreamURL found
            uint16_t len = idx2 - idx1;
            sUrl.assign(ml + idx1, len);
            if(sUrl.valid()){
                AUDIO_INFO("%s", sUrl.get());
            }
        }
    }

    if(indexOf(ml, "adw_ad=", 0) > 0){
        idx1 = indexOf(ml, "adw_ad=", 0);
        if(idx1 >= 0) { // Advertisement found
            idx1 = indexOf(ml, "durationMilliseconds=", 0);
            idx2 = indexOf(ml, ";", idx1);
            if(idx1 >= 0 && idx2 > idx1) {
                uint16_t len = idx2 - idx1;
                ps_ptr<char> sAdv;
                sAdv.assign(ml + idx1, len + 1);
                AUDIO_INFO("%s", sAdv.get());
                uint8_t pos = 21;                                                 // remove "StreamTitle="
                if(sAdv[pos] == '\'') pos++;                                      // remove leading  \'
                if(sAdv[strlen(sAdv.get()) - 1] == '\'') sAdv[strlen(sAdv.get()) - 1] = '\0'; // remove trailing \'
                if(audio_commercial) audio_commercial(sAdv.get() + pos);
            }
        }
    }

    if(streamTitle.valid()){
        if(!m_streamTitle.valid()){
            m_streamTitle.clone_from(streamTitle); // first init
            if(audio_showstreamtitle) audio_showstreamtitle(streamTitle.get());
        }
        if(!m_streamTitle.equals(streamTitle)){
            if(audio_showstreamtitle) audio_showstreamtitle(streamTitle.get());
            m_streamTitle.clone_from(streamTitle);
        }
    }
}
//##################################################################
/*void Audio::showCodecParams() {
    // print Codec Parameter (mp3, aac) in audio_info()

    AUDIO_INFO("Channels: %u", getChannels());
    AUDIO_INFO("SampleRate: %lu", getSampleRate());
    AUDIO_INFO("BitsPerSample: %u", getBitsPerSample());
    if(getBitRate()) { AUDIO_INFO("BitRate: %lu", m_bitRate); }
    else { AUDIO_INFO("BitRate: N/A"); }

    if(m_codec == CODEC_AAC) {
        uint8_t answ = AACGetFormat();
        if(answ < 3) {
            const char hf[4][8] = {"unknown", "ADIF", "ADTS"};
            AUDIO_INFO("AAC HeaderFormat: %s", hf[answ])
        }
        answ = AACGetSBR();
        if(answ > 0 && answ < 4) {
            const char sbr[4][50] = {"without SBR", "upsampled SBR", "downsampled SBR", "no SBR used, but file is upsampled by a factor 2"};
            AUDIO_INFO("Spectral band replication: %s", sbr[answ]);
        }
    }
}*/
//###################################################################
size_t Audio::sendBytes(uint8_t* data, size_t len){

   if(!m_f_running) return 0; // guard

/*     m_sbyt.bytesLeft = 0;
    m_sbyt.nextSync = 0;

    if(!m_f_playing) {
        m_sbyt.channels = 2; // assume aac stereo
//        m_sbyt.isPS = 0;
        m_sbyt.f_setDecodeParamsOnce = true;
        m_sbyt.nextSync = findNextSync(data, len);
        if(m_sbyt.nextSync <  0) return len; // no syncword found
        if(m_sbyt.nextSync == 0) { m_f_playing = true; }
        if(m_sbyt.nextSync >  0) return m_sbyt.nextSync;
    }
    // m_f_playing is true at this pos
    m_sbyt.bytesLeft = len;
    int bytesDecoded = 0;

    if(m_codec == CODEC_NONE && m_playlistFormat == FORMAT_M3U8) return 0; // can happen when the m3u8 playlist is loaded
    if(!m_f_decode_ready) return 0; // find sync first

    bytesDecoded = len - m_sbyt.bytesLeft;

    if(bytesDecoded == 0) { // unlikely framesize
        AUDIO_INFO("framesize is 0, start decoding again");
        m_f_playing = false; // seek for new syncword
        // we're here because there was a wrong sync word so skip one byte and seek for the next
        return 1;
    }	*/
    // status: bytesDecoded > 0
    const char* st = NULL;
    std::vector<uint32_t> vec;
    switch(m_codec) {
        case CODEC_FLAC:
                            st = FLACgetStreamTitle();
                            if(st) {
                                AUDIO_INFO(st);
                                if(audio_showstreamtitle) audio_showstreamtitle(st);
                            }
                            break;
        case CODEC_VORBIS:
                            st = VORBISgetStreamTitle();
                            if(st) {
                                AUDIO_INFO(st);
                                if(audio_showstreamtitle) audio_showstreamtitle(st);
                            }
                            break;
/*        case CODEC_OPUS:
                            st = OPUSgetStreamTitle();
                            if(st){
                                AUDIO_INFO(st);
                                if(audio_showstreamtitle) audio_showstreamtitle(st);
                            }
                            break;
*/
    }
/*    if(m_sbyt.f_setDecodeParamsOnce && m_validSamples) {
        m_sbyt.f_setDecodeParamsOnce = false;
        m_PlayingStartTime = millis();
    }

    uint16_t bytesDecoderOut = m_validSamples;
    computeAudioTime(bytesDecoded, bytesDecoderOut);
//    computeAudioTime(bytesDecoded, m_validSamples);

//    m_curSample = 0;
//    playChunk();
//    return bytesDecoded;
//}
//##################################################################
//void Audio::playChunk() {
//    if(m_validSamples == 0) return 0; // nothing to do

//    if(!m_f_running) return 0; // guard
*/
    size_t chunk_length = 0;                                // Length of chunk 32 byte or shorter
    size_t bytesDecoded = 0;
//    bytesDecoded = 0;
//    uint8_t* data = InBuff.getReadPtr();

    data_mode_on();
    while(len){                                             // More to do?
        if(!digitalRead(dreq_pin)) break;
        chunk_length = len;
        if(len > vs1053_chunk_size){
            chunk_length = vs1053_chunk_size;
        }
        m_PlayingStartTime = millis();
        spi_VS1053->writeBytes(data, chunk_length);
        data         += chunk_length;
        len          -= chunk_length;
        bytesDecoded += chunk_length;
    }
    data_mode_off();
    return bytesDecoded;
//    return;
}
//##################################################################
/*void Audio::computeAudioTime(uint16_t bytesDecoderIn, uint16_t bytesDecoderOut) {

    if(m_dataMode != AUDIO_LOCALFILE && m_streamType != ST_WEBFILE) return; //guard

    if(m_f_firstCurTimeCall) { // first call
        m_f_firstCurTimeCall = false;
        m_cat.sumBytesIn = 0;
        m_cat.sumBytesOut = 0;
        m_cat.sumBitRate  = 0;
        m_cat.counter = 0;
        m_cat.timeStamp = millis();
        m_cat.deltaBytesIn = 0;
        m_cat.nominalBitRate = 0;

        if(m_codec == CODEC_FLAC && getAudioFileDuration()){
//            m_audioFileDuration = FLACGetAudioFileDuration();
            m_cat.nominalBitRate = (m_audioDataSize / m_audioFileDuration) * 8;
            m_avr_bitrate = m_cat.nominalBitRate;
        }
//        if(m_codec == CODEC_WAV){
//            m_cat.nominalBitRate = getBitRate();
//            m_avr_bitrate = m_cat.nominalBitRate;
//            m_audioFileDuration = m_audioDataSize  / (getSampleRate() * getChannels());
//            if(getBitsPerSample() == 16) m_audioFileDuration /= 2;
//        }
    }

    m_cat.sumBytesIn   += bytesDecoderIn;
    m_cat.deltaBytesIn += bytesDecoderIn;
    m_cat.sumBytesOut  += bytesDecoderOut;


    if(m_cat.timeStamp + 500 < millis()){
        uint32_t t       = millis();      // time tracking
        uint32_t delta_t = t - m_cat.timeStamp; //    ---"---
        m_cat.timeStamp = t;                    //    ---"---

        uint32_t bitRate = ((m_cat.deltaBytesIn * 8000) / delta_t);  // we know the time and bytesIn to compute the bitrate

        m_cat.sumBitRate += bitRate;
        m_cat.counter ++;
        if(m_cat.nominalBitRate){
            m_audioCurrentTime = round(((float)m_cat.sumBytesIn * 8) / m_avr_bitrate);
        }
        else{
            m_avr_bitrate = m_cat.sumBitRate / m_cat.counter;
            m_audioCurrentTime = (m_cat.sumBytesIn * 8) / m_avr_bitrate;
            m_audioFileDuration = round(((float)m_audioDataSize * 8 / m_avr_bitrate));
        }
        m_cat.deltaBytesIn = 0;
    }

    if(m_haveNewFilePos && m_avr_bitrate){
        uint32_t posWhithinAudioBlock =  m_haveNewFilePos;
        uint32_t newTime = posWhithinAudioBlock / (m_avr_bitrate / 8);
        m_audioCurrentTime = newTime;
        m_cat.sumBytesIn = posWhithinAudioBlock;
        m_haveNewFilePos = 0;
    }
}*/
//****************************************************************************************
uint32_t Audio::getFileSize() { // returns the size of webfile or local file
    if(!m_audiofile) {
        if (m_contentlength > 0) { return m_contentlength;}
        return 0;
    }
    return m_audiofile.size();
}
//###################################################################
uint32_t Audio::getFilePos() {
    if(m_dataMode == AUDIO_LOCALFILE){
        if(!m_audiofile) return 0;
        return m_audiofile.position();
    }
    if(m_streamType == ST_WEBFILE){
        return m_webFilePos;
    }
    return 0;
}
//###################################################################
uint32_t Audio::getAudioDataStartPos() {
    if(m_dataMode == AUDIO_LOCALFILE){
        if(!m_audiofile) return 0;
        return m_audioDataStart;
    }
    if(m_streamType == ST_WEBFILE){
        return m_audioDataStart;
    }
    return 0;
}
//###################################################################
uint32_t Audio::getAudioFileDuration(){
    if(m_playlistFormat == FORMAT_M3U8)                     return 0;
    if(m_dataMode == AUDIO_LOCALFILE) {if(!m_audioDataSize) return 0;}
    if(m_streamType == ST_WEBFILE)    {if(!m_contentlength) return 0;}

    if(!m_avr_bitrate) return 0;
    else
      m_audioFileDuration = 8 * (m_audioDataSize / m_avr_bitrate);

    return m_audioFileDuration;
}
//###################################################################
/*uint32_t Audio::getAudioCurrentTime() { // return current time in seconds
    return round(m_audioCurrentTime);
}*/
   const uint16_t l3id012[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
   const uint16_t l3id3[16]   = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
uint32_t Audio::getAudioCurrentTime(){

  if(m_dataMode != AUDIO_LOCALFILE && m_streamType != ST_WEBFILE) return 0; //guard

  uint16_t SCIStatus;
  uint32_t prev_bitrate = m_avr_bitrate;

  if(m_codec == CODEC_MP3){
    SCIStatus=read_register(SCI_HDAT1);
    uint8_t layer = SCIStatus>>1 & 0b11;
    uint8_t id    = SCIStatus>>3 & 0b11;
    SCIStatus = read_register(SCI_HDAT0);
    if(layer==1){ //layer3
       if(id==3){m_avr_bitrate = l3id3[SCIStatus>>12] * 1000;}
       else{       m_avr_bitrate = l3id012[SCIStatus>>12] * 1000;}
       m_localBitrateSend = prev_bitrate==m_avr_bitrate;
       if(m_avr_bitrate==0) return 0;
       sprintf(m_chbuf, "%d", m_avr_bitrate);
       if(audio_bitrate && !m_localBitrateSend) audio_bitrate(m_chbuf);
       m_localBitrateSend = true;
       m_audioFileDuration = 8 * ((float)m_audioDataSize / (m_avr_bitrate));
       m_audioCurrentTime = 8 * ((float)getFilePos() / (m_avr_bitrate));

       if(m_haveNewFilePos && m_avr_bitrate){
           uint32_t posWhithinAudioBlock =  m_haveNewFilePos - m_audioDataStart;
           uint32_t newTime = posWhithinAudioBlock / (m_avr_bitrate / 8);
           m_audioCurrentTime = newTime;
//           sumBytesIn = posWhithinAudioBlock;
           m_haveNewFilePos = 0;
       }
       return round(m_audioCurrentTime);
    }
    return 0;
  }

  if(m_codec == CODEC_AAC || m_codec == CODEC_FLAC || m_codec == CODEC_WAV || m_codec == CODEC_M4A || m_codec == CODEC_VORBIS || m_codec == CODEC_OPUS){
    SCIStatus=read_register(SCI_HDAT0);
    m_avr_bitrate = SCIStatus * 8;
    m_localBitrateSend = prev_bitrate==m_avr_bitrate;
    if(m_avr_bitrate==0) return 0;
    sprintf(m_chbuf, "%d", m_avr_bitrate);
    if(audio_bitrate && !m_localBitrateSend) audio_bitrate(m_chbuf);
    m_localBitrateSend = true;
    m_audioFileDuration = 8 * ((float)m_audioDataSize / (m_avr_bitrate));
    m_audioCurrentTime = 8 * ((float)getFilePos() / (m_avr_bitrate));

    if(m_haveNewFilePos && m_avr_bitrate){
        uint32_t posWhithinAudioBlock =  m_haveNewFilePos - m_audioDataStart;
        uint32_t newTime = posWhithinAudioBlock / (m_avr_bitrate / 8);
        m_audioCurrentTime = newTime;
//        sumBytesIn = posWhithinAudioBlock;
        m_haveNewFilePos = 0;
    }
    return round(m_audioCurrentTime);
  }
  
    return 0;
}
//###################################################################
bool Audio::setAudioPlayPosition(uint16_t sec) {
//    if(!m_f_psramFound) {               log_w("PSRAM must be activated"); return false;} // guard
    if(m_dataMode != AUDIO_LOCALFILE /* && m_streamType == ST_WEBFILE */) return false;  // guard
    if(!m_avr_bitrate)                                                    return false;  // guard
    //if(m_codec == CODEC_OPUS) return false;   // not impl. yet
    //if(m_codec == CODEC_VORBIS) return false; // not impl. yet
    // Jump to an absolute position in time within an audio file
    // e.g. setAudioPlayPosition(300) sets the pointer at pos 5 min
    if(sec > getAudioFileDuration()) sec = getAudioFileDuration();
    uint32_t filepos = m_audioDataStart + (m_avr_bitrate * sec / 8);
    if(m_dataMode == AUDIO_LOCALFILE) return setFilePos(filepos);
//    if(m_streamType == ST_WEBFILE) return httpRange(m_lastHost, filepos);
    return false;
}
//###################################################################
uint32_t Audio::getTotalPlayingTime() {
    // Is set to zero by a connectToXXX() and starts as soon as the first audio data is available,
    // the time counting is not interrupted by a 'pause / resume'
    return millis() - m_PlayingStartTime;
}
//###################################################################
bool Audio::setTimeOffset(int sec) { // fast forward or rewind the current position in seconds
    if(!m_f_psramFound) {               log_w("PSRAM must be activated"); return false;} // guard
    if(m_dataMode != AUDIO_LOCALFILE /* && m_streamType == ST_WEBFILE */) return false;  // guard
    if(m_dataMode == AUDIO_LOCALFILE && !m_audiofile)                       return false;  // guard
    if(!m_avr_bitrate)                                                    return false;  // guard
    if(m_codec == CODEC_AAC) return false; // not impl. yet
    uint32_t oneSec = m_avr_bitrate / 8;                 // bytes decoded in one sec
    int32_t  offset = oneSec * sec;                      // bytes to be wind/rewind
//    uint32_t startAB = m_audioDataStart; 		// audioblock begin
//    uint32_t endAB = m_audioDataStart + m_audioDataSize; 		// audioblock end
    int32_t pos = getFilePos() - inBufferFilled();
    pos += offset;
    if(pos < (int32_t)m_audioDataStart) {pos = m_audioDataStart;}
    if(pos >= (int32_t)(m_audioDataStart + m_audioDataSize))  {pos = m_audioDataStart + m_audioDataSize;}
    setFilePos(pos);

    return true;
}
//###################################################################
bool Audio::setFilePos(uint32_t pos) {
    if(!m_f_psramFound) {               log_w("PSRAM must be activated"); return false;} // guard
    if(m_dataMode != AUDIO_LOCALFILE /* && m_streamType == ST_WEBFILE */) return false;  // guard
    if(m_dataMode == AUDIO_LOCALFILE && !m_audiofile)                       return false;  // guard
    if(m_codec == CODEC_AAC)                                              return false;  // guard, not impl. yet

//    uint32_t startAB = m_audioDataStart;                 // audioblock begin
//    uint32_t endAB = m_audioDataStart + m_audioDataSize; // audioblock end
    if(pos < (int32_t)m_audioDataStart) {pos = m_audioDataStart;}
    if(pos >= (int32_t)(m_audioDataStart + m_audioDataSize))  {pos = m_audioDataStart + m_audioDataSize;}


    m_validSamples = 0;
    if(m_dataMode == AUDIO_LOCALFILE /* || m_streamType == ST_WEBFILE */) {
        m_resumeFilePos = pos;  // used in processLocalFile()
        return true;
//        return m_audiofile.seek(pos);
    }
    return false;
}
//###################################################################
bool Audio::setBitrate(uint32_t br) {
    m_bitRate = br;
    if(br) return true;
    return false;
}

/*uint32_t Audio::getBitRate(bool avg) {
    if(avg&&m_avr_bitrate)  m_bitRate = m_avr_bitrate;
    else if(m_bitRate == 0) m_bitRate = (wram_read(0x1e05) & 0xFF) * 1000;  	// Kbit/s => bit/s
    return m_bitRate;
}	*/
//###################################################################
const uint8_t everyn = 4;
void Audio::computeVUlevel() {

// * \brief get current measured VU Meter
// * Returns the calculated peak sample values from both channels in 3 dB increaments through.
// * Where the high byte represent the left channel, and the low bytes the right channel.
// * Values from 0 to 31 are valid for both channels.
// *
// * \warning This feature is only available with patches that support VU meter.

//  if(!VS_PATCH_ENABLE) return;
  static uint8_t cc = 0;
  cc++;
  if(!_vuInitalized || !config.store.vumeter || cc!=everyn) return;
  if(cc==everyn) cc=0;
  int16_t reg = read_register(SCI_AICTRL3); 	 // returns the values in 1 dB resolution from 0 (lowest) 95 (highest)
  vuLeft = map((uint8_t)(reg & 0x00FF), 85, 95, 0, 255);
  vuRight = map((uint8_t)(reg >> 8), 85, 95, 0, 255);
  if(vuLeft>config.vuThreshold) config.vuThreshold = vuLeft;
  if(vuRight>config.vuThreshold) config.vuThreshold=vuRight;
}

//###################################################################
uint16_t Audio::get_VUlevel(uint16_t dimension){
//  if(!VS_PATCH_ENABLE) return 0;
  if(!_vuInitalized || !config.store.vumeter || config.vuThreshold==0 || !m_f_running) return 0;
  uint8_t L = map(vuLeft, config.vuThreshold, 0, 0, dimension);
  uint8_t R = map(vuRight, config.vuThreshold, 0, 0, dimension);
  return (L << 8) | R;
}
//###################################################################
/*size_t Audio::bufferFilled(){
    return InBuff.bufferFilled();
}
//###################################################################
size_t Audio::bufferFree(){
    return InBuff.freeSpace();
}	*/
//###################################################################
void Audio::setTone(int8_t *rtone){                       // Set bass/treble (4 nibbles)

    // Set tone characteristics.  See documentation for the 4 nibbles.
    uint16_t value=0;                                       // Value to send to SCI_BASS
    int i;                                                  // Loop control

    for(i=0; i < 4; i++) {
        value=(value << 4) | rtone[i];                      // Shift next nibble in
    }
    write_register(SCI_BASS, value);                        // Volume left and right
}
/*
Name			Bits			Description
ST AMPLITUDE		15:12		Treble Control in 1.5 dB steps (-8..7, 0 = off)
ST FREQLIMIT		11:8			Lower limit frequency in 1000 Hz steps (1..15) 			// 1000..15000
SB AMPLITUDE		7:4			Bass Enhancement in 1 dB steps (0..15, 0 = off)
SB FREQLIMIT		3:0			Lower limit frequency in 10 Hz steps (2..15)				// 20..150
*/
/*
void Audio::setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass){ 
  if(gainLowPass<0) gainLowPass=0;
  if(gainLowPass>15) gainLowPass=15;
  if(gainBandPass<0) gainBandPass=0;
  if(gainBandPass>13) gainBandPass=13;
  int8_t rtone[] = {(int8_t)map(gainHighPass, -16, 16, -8, 7), (int8_t)(2+gainBandPass), gainLowPass, (int8_t)(15-gainBandPass)};
  setTone(rtone);
}
*/
void Audio::setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass){ 
  gainHighPass = constrain(gainHighPass, -16, 16);
  gainBandPass = constrain(gainBandPass, -16, 16);
  gainLowPass = constrain(gainLowPass, -16, 16);

  uint8_t trebleFreqLimit = map(-gainBandPass, -16, 16, 1, 15);
  uint8_t bassFreqLimit = map(gainBandPass, -16, 16, 2, 15);
  uint8_t st_amplitude = map(gainHighPass, -16, 16, -8, 7);
  uint8_t sb_amplitude = map(gainLowPass, -16, 16, 0, 15);
  uint16_t sci_bass = (st_amplitude << 12) | (trebleFreqLimit << 8) |
                      (sb_amplitude << 4) | bassFreqLimit;
  write_register(SCI_BASS, sci_bass);
}
//###################################################################
void Audio::forceMono(bool m) { // #100 mono option
  m_f_forceMono = m;          // false stereo, true mono
  if (m_f_forceMono != false) {
//    wram_write(0x1e09, 0x0001);		// To activate the mono downmix mode
      write_register(SCI_WRAMADDR, 0x1e09);
      write_register(SCI_WRAM, 0x0001);
  }
}
//###################################################################
void Audio::setBalance(int8_t bal) { // bal -16...16
//        if (bal > 16) { bal = 16;
//        if (bal < -16) { bal = -16;
        m_balance = bal;

	setVolume(curvol);
}
//###################################################################
void Audio::setVolume(uint8_t vol){
    // Set volume.  Both left and right.
    // Input value is 0..21.  21 is the loudest.
    // Clicking reduced by using 0xf8 to 0x00 as limits.
//    uint16_t value;					// Value to send to SCI_VOL
	uint8_t valueL, valueR;			// Value to send to SCI_VOL
	int16_t balance_map = map(m_balance, -16, 16, -100, 100);
	
        curvol = vol;					// Save for later use
	valueL = vol;
   	valueR = vol;
    if (balance_map < 0) {
        valueR = (float)valueR-(float)valueR * abs((float)balance_map)/100;
    } else if (balance_map > 0) {
    		valueL = (float)valueL-(float)valueL * abs((float)balance_map)/100;
    }
	uint8_t lgvolL = VS1053VOL(valueL);
	uint8_t lgvolR = VS1053VOL(valueR);
	if(lgvolL==VS1053VOLM) lgvolL=0;
	if(lgvolR==VS1053VOLM) lgvolR=0;
	valueL=map(lgvolL, 0, 254, 0xF8, 0x00);
	valueR=map(lgvolR, 0, 254, 0xF8, 0x00);
//	value=(valueL << 8) | valueR;
	write_register(SCI_VOL, (valueL << 8) | valueR);
}
//###################################################################
uint8_t Audio::getVolume(){                                 // Get the currenet volume setting.
    return curvol;
}
//###################################################################
//###################################################################
uint32_t Audio::inBufferFilled() {
    // current audio input buffer fillsize in bytes
    return InBuff.bufferFilled();
}
/*uint32_t Audio::inBufferFree() {
    // current audio input buffer free space in bytes
    return InBuff.freeSpace();
}

uint32_t Audio::getInBufferSize() {
    // current audio input buffer size in bytes
    return InBuff.getBufsize();
}
bool Audio::setInBufferSize(size_t mbs){
    size_t oldBuffSize = InBuff.getBufsize();
    stopSong();
    bool res = InBuff.setBufsize(mbs);
    if(res == false) InBuff.setBufsize(oldBuffSize);
    if(res == false){
        AUDIO_ERROR("%i bytes is not possible, back to %i bytes", mbs, oldBuffSize);
        InBuff.setBufsize(oldBuffSize);
    }
    InBuff.init();
    return res;
}	*/
//###################################################################
//    AAC - T R A N S P O R T S T R E A M
//###################################################################
bool Audio::ts_parsePacket(uint8_t* packet, uint8_t* packetStart, uint8_t* packetLength) {

    bool log = false;

    const uint8_t TS_PACKET_SIZE = 188;
    const uint8_t PAYLOAD_SIZE = 184;
    const uint8_t PID_ARRAY_LEN = 4;

    (void)PAYLOAD_SIZE; // suppress [-Wunused-variable]

    if(packet == NULL) {
        if(log) log_w("parseTS reset");
        for(int i = 0; i < PID_ARRAY_LEN; i++) m_tspp.pids[i] = 0;
        m_tspp.PES_DataLength = 0;
        m_tspp.pidOfAAC = 0;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------
    // 0. Byte SyncByte  | 0 | 1 | 0 | 0 | 0 | 1 | 1 | 1 | always bit pattern of 0x47
    //---------------------------------------------------------------------------------------------------------
    // 1. Byte           |PUSI|TP|   |PID|PID|PID|PID|PID|
    //---------------------------------------------------------------------------------------------------------
    // 2. Byte           |PID|PID|PID|PID|PID|PID|PID|PID|
    //---------------------------------------------------------------------------------------------------------
    // 3. Byte           |TSC|TSC|AFC|AFC|CC |CC |CC |CC |
    //---------------------------------------------------------------------------------------------------------
    // 4.-187. Byte      |Payload data if AFC==01 or 11  |
    //---------------------------------------------------------------------------------------------------------

    // PUSI Payload unit start indicator, set when this packet contains the first byte of a new payload unit.
    //      The first byte of the payload will indicate where this new payload unit starts.
    // TP   Transport priority, set when the current packet has a higher priority than other packets with the same PID.
    // PID  Packet Identifier, describing the payload data.
    // TSC  Transport scrambling control, '00' = Not scrambled.
    // AFC  Adaptation field control, 01 – no adaptation field, payload only, 10 – adaptation field only, no payload,
    //                                11 – adaptation field followed by payload, 00 – RESERVED for future use
    // CC   Continuity counter, Sequence number of payload packets (0x00 to 0x0F) within each stream (except PID 8191)

    // for(int i = 1; i < 188; i++) {printf("%02X ", packet[i - 1]); if(i && (i % 16 == 0)) printf("\n");}
    // printf("\n----------\n");

    if(packet[0] != 0x47) {
        AUDIO_ERROR("ts SyncByte not found, first bytes are 0x%02X 0x%02X 0x%02X 0x%02X", packet[0], packet[1], packet[2], packet[3]);
        // stopSong();
        return false;
    }
    int PID = (packet[1] & 0x1F) << 8 | (packet[2] & 0xFF);
    if(log) log_w("PID: 0x%04X(%d)", PID, PID);
    int PUSI = (packet[1] & 0x40) >> 6;
    if(log) log_w("Payload Unit Start Indicator: %d", PUSI);
    int AFC = (packet[3] & 0x30) >> 4;
    if(log) log_w("Adaption Field Control: %d", AFC);

    int AFL = -1;
    if((AFC & 0b10) == 0b10) {  // AFC '11' Adaptation Field followed
        AFL = packet[4] & 0xFF; // Adaptation Field Length
        if(log) log_w("Adaptation Field Length: %d", AFL);
    }
    int PLS = PUSI ? 5 : 4;     // PayLoadStart, Payload Unit Start Indicator
    if(AFL > 0) PLS += AFL + 1; // skip adaption field

    if(AFC == 2){ // The TS package contains only an adaptation Field and no user data.
        *packetStart = AFL + 1;
        *packetLength = 0;
        return true;
    }

    if(PID == 0) {
        // Program Association Table (PAT) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(log) log_w("PAT");
        m_tspp.pidNumber = 0;
        m_tspp.pidOfAAC = 0;

        int startOfProgramNums = 8;
        int lengthOfPATValue = 4;
        int sectionLength = ((packet[PLS + 1] & 0x0F) << 8) | (packet[PLS + 2] & 0xFF);
        if(log) log_w("Section Length: %d", sectionLength);
        int program_number, program_map_PID;
        int indexOfPids = 0;
        (void)program_number; // [-Wunused-but-set-variable]
        for(int i = startOfProgramNums; i <= sectionLength; i += lengthOfPATValue) {
            program_number = ((packet[PLS + i] & 0xFF) << 8) | (packet[PLS + i + 1] & 0xFF);
            program_map_PID = ((packet[PLS + i + 2] & 0x1F) << 8) | (packet[PLS + i + 3] & 0xFF);
            if(log) log_w("Program Num: 0x%04X(%d) PMT PID: 0x%04X(%d)", program_number, program_number, program_map_PID, program_map_PID);
            m_tspp.pids[indexOfPids++] = program_map_PID;
        }
        m_tspp.pidNumber = indexOfPids;
        *packetStart = 0;
        *packetLength = 0;
        return true;
    }
    else if(PID == m_tspp.pidOfAAC) {
        if(log) log_w("AAC");
        uint8_t posOfPacketStart = 4;
        if(AFL >= 0) {
            posOfPacketStart = 5 + AFL;
            if(log) log_w("posOfPacketStart: %d", posOfPacketStart);
        }
        // Packetized Elementary Stream (PES) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(log) log_w("PES_DataLength %i", m_tspp.PES_DataLength);
        if(m_tspp.PES_DataLength > 0) {
            *packetStart = posOfPacketStart + m_tspp.fillData;
            *packetLength = TS_PACKET_SIZE - posOfPacketStart - m_tspp.fillData;
            if(log) log_w("packetlength %i", *packetLength);
            m_tspp.fillData = 0;
            m_tspp.PES_DataLength -= (*packetLength);
            return true;
        }
        else {
            int firstByte = packet[posOfPacketStart] & 0xFF;
            int secondByte = packet[posOfPacketStart + 1] & 0xFF;
            int thirdByte = packet[posOfPacketStart + 2] & 0xFF;
            if(log) log_w ("First 3 bytes: 0x%02X, 0x%02X, 0x%02X", firstByte, secondByte, thirdByte);
            if(firstByte == 0x00 && secondByte == 0x00 && thirdByte == 0x01) { // Packet start code prefix
                // --------------------------------------------------------------------------------------------------------
                // posOfPacketStart + 0...2     0x00, 0x00, 0x01                                          PES-Startcode
                //---------------------------------------------------------------------------------------------------------
                // posOfPacketStart + 3         0xE0 (Video) od 0xC0 (Audio)                              StreamID
                //---------------------------------------------------------------------------------------------------------
                // posOfPacketStart + 4...5     0xLL, 0xLL                                                PES Packet length
                //---------------------------------------------------------------------------------------------------------
                // posOfPacketStart + 6...7                                                               PTS/DTS Flags
                //---------------------------------------------------------------------------------------------------------
                // posOfPacketStart + 8         0xXX                                                      header length
                //---------------------------------------------------------------------------------------------------------

                uint8_t StreamID = packet[posOfPacketStart + 3] & 0xFF;
                if(StreamID >= 0xC0 && StreamID <= 0xDF) { ; } // okay ist audio stream
                if(StreamID >= 0xE0 && StreamID <= 0xEF) {AUDIO_ERROR("video stream!"); return false; }
                int PES_PacketLength = ((packet[posOfPacketStart + 4] & 0xFF) << 8) + (packet[posOfPacketStart + 5] & 0xFF);
                if(log) log_w("PES PacketLength: %d", PES_PacketLength);
                bool PTS_flag = false;
                bool DTS_flag = false;
                int flag_byte1 = packet[posOfPacketStart + 6] & 0xFF;
                int flag_byte2 = packet[posOfPacketStart + 7] & 0xFF;  (void) flag_byte2; // unused yet
                if(flag_byte1 & 0b10000000) PTS_flag = true;
                if(flag_byte1 & 0b00000100) DTS_flag = true;
                if(log && PTS_flag) log_w("PTS_flag is set");
                if(log && DTS_flag) log_w("DTS_flag is set");
                uint8_t PES_HeaderDataLength = packet[posOfPacketStart + 8] & 0xFF;
                if(log) log_w("PES_headerDataLength %d", PES_HeaderDataLength);

                m_tspp.PES_DataLength = PES_PacketLength;
                int startOfData = PES_HeaderDataLength + 9;
                if(posOfPacketStart + startOfData >= 188) { // only fillers in packet
                    if(log) log_w("posOfPacketStart + startOfData %i", posOfPacketStart + startOfData);
                    *packetStart = 0;
                    *packetLength = 0;
                    m_tspp.PES_DataLength -= (PES_HeaderDataLength + 3);
                    m_tspp.fillData = (posOfPacketStart + startOfData) - 188;
                    if(log) log_w("fillData %i", m_tspp.fillData);
                    return true;
                }
                if(log) log_w("First AAC data byte: %02X", packet[posOfPacketStart + startOfData]);
                if(log) log_w("Second AAC data byte: %02X", packet[posOfPacketStart + startOfData + 1]);
                *packetStart = posOfPacketStart + startOfData;
                *packetLength = TS_PACKET_SIZE - posOfPacketStart - startOfData;
                m_tspp.PES_DataLength -= (*packetLength);
                m_tspp.PES_DataLength -= (PES_HeaderDataLength + 3);
                return true;
            }
            if(firstByte == 0 && secondByte == 0 && thirdByte == 0){
                // PES packet startcode prefix is 0x000000
                // skip such packets
                return true;
            }
        }
        *packetStart = 0;
        *packetLength = 0;
        AUDIO_ERROR("PES not found");
        return false;
    }
    else if(m_tspp.pidNumber) {
        //  Program Map Table (PMT) - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        for(int i = 0; i < m_tspp.pidNumber; i++) {
            if(PID == m_tspp.pids[i]) {
                if(log) log_w("PMT");
                int staticLengthOfPMT = 12;
                int sectionLength = ((packet[PLS + 1] & 0x0F) << 8) | (packet[PLS + 2] & 0xFF);
                if(log) log_w("Section Length: %d", sectionLength);
                int programInfoLength = ((packet[PLS + 10] & 0x0F) << 8) | (packet[PLS + 11] & 0xFF);
                if(log) log_w("Program Info Length: %d", programInfoLength);
                int cursor = staticLengthOfPMT + programInfoLength;
                while(cursor < sectionLength - 1) {
                    int streamType = packet[PLS + cursor] & 0xFF;
                    int elementaryPID = ((packet[PLS + cursor + 1] & 0x1F) << 8) | (packet[PLS + cursor + 2] & 0xFF);
                    if(log) log_w("Stream Type: 0x%02X Elementary PID: 0x%04X", streamType, elementaryPID);

                    if(streamType == 0x0F || streamType == 0x11 || streamType == 0x04) {
                        if(log) log_w("AAC PID discover");
                        m_tspp.pidOfAAC = elementaryPID;
                    }
                    int esInfoLength = ((packet[PLS + cursor + 3] & 0x0F) << 8) | (packet[PLS + cursor + 4] & 0xFF);
                    if(log) log_w("ES Info Length: 0x%04X", esInfoLength);
                    cursor += 5 + esInfoLength;
                }
            }
        }
        *packetStart = 0;
        *packetLength = 0;
        return true;
    }
    // PES received before PAT and PMT seen
    *packetStart = 0;
    *packetLength = 0;
    return false;
}
//##################################################################
//    W E B S T R E A M  -  H E L P   F U N C T I O N S
//##################################################################
uint16_t Audio::readMetadata(uint16_t maxBytes, bool first) {
    m_rmet.res = 0;
    ps_ptr<char>buff; buff.alloc(4096); buff.clear(); // is max 256 *16
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(first) {
        m_rmet.pos_ml = 0; // determines the current position in metaline
        m_rmet.metalen = 0;
        return 0;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    if(!maxBytes) return 0; // guard

    if(!m_rmet.metalen) {
        int b = _client->read(); // First byte of metadata?
        if (b < 0) {
            AUDIO_INFO("client->read() failed (%d)", b);
            return 0;
        }
        m_rmet.metalen = b * 16;        // New count for metadata including length byte, max 4096
        m_rmet.pos_ml = 0;
        buff[m_rmet.pos_ml] = 0; // Prepare for new line
        m_rmet.res = 1;
    }
    if(!m_rmet.metalen) {
        m_metacount = m_metaint;
        return m_rmet.res;
    } // metalen is 0
    uint16_t a = _client->readBytes(&buff[m_rmet.pos_ml], min((uint16_t)(m_rmet.metalen - m_rmet.pos_ml), (uint16_t)(maxBytes)));
    m_rmet.res += a;
    m_rmet.pos_ml += a;

    if(m_rmet.pos_ml == m_rmet.metalen) {
        buff[m_rmet.pos_ml] = '\0';
        if(buff.strlen() > 0) { // Any info present?
            // metaline contains artist and song name.  For example:
            // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
            // Sometimes it is just other info like:
            // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
            // Isolate the StreamTitle, remove leading and trailing quotes if present.
            latinToUTF8(buff);          // convert to UTF-8 if necessary
            int pos = buff.index_of_icase("song_spot", 0); // remove some irrelevant infos
            if(pos > 3) {                               // e.g. song_spot="T" MediaBaseId="0" itunesTrackId="0"
                buff[pos] = 0;
            }
            showstreamtitle(buff.get()); // Show artist and title if present in metadata
        }
        m_metacount = m_metaint;
        m_rmet.metalen = 0;
        m_rmet.pos_ml = 0;
    }
    return m_rmet.res;
}
//##################################################################
size_t Audio::readChunkSize(uint8_t* bytes) {
    uint8_t  byteCounter = 0;
    size_t   chunksize = 0;
    // bool     parsingChunkSize = true;
    int      b = 0;
    std::string chunkLine;
    uint32_t ctime = millis();
    uint32_t timeout = 2000; // ms

    while (true) {
        if ((millis() - ctime) > timeout) {
            AUDIO_ERROR("chunkedDataTransfer: timeout");
            stopSong();
            return 0;
        }
        if(!_client->available()) continue; // no data available yet
        b = _client->read();
        if (b < 0) continue; // -1 = no data

        byteCounter++;
        if (b == '\n') break; // End of chunk-size line
        if (b == '\r') continue;

        chunkLine += static_cast<char>(b);
    }

    // chunkLine z.B.: "2A", oder "2A;foo=bar"
    size_t semicolonPos = chunkLine.find(';');
    std::string hexSize = (semicolonPos != std::string::npos) ? chunkLine.substr(0, semicolonPos) : chunkLine;

    // Converted hex number
    chunksize = strtoul(hexSize.c_str(), nullptr, 16);
    *bytes = byteCounter;

    if (chunksize == 0) { // Special case: Last chunk recognized (0) => Next read and reject "\ r \ n"
        // Reading to complete "\ r \ n" was received
        uint8_t crlf[2] = {0}; (void)crlf; // suppress [-Wunused-variable]
        uint8_t idx = 0;
        ctime = millis();
        while (idx < 2 && (millis() - ctime) < timeout) {
            int ch = _client->read();
            if (ch < 0) continue;
            crlf[idx++] = static_cast<uint8_t>(ch);
        }
    }

    return chunksize;
}
//##################################################################
bool Audio::readID3V1Tag() {
    if (m_codec != CODEC_MP3) return false;
    ps_ptr<char>chBuff;
    chBuff.alloc(256);

    const uint8_t* p = InBuff.getReadPtr();

    // Lambda for simplification
    auto readID3Field = [&](ps_ptr<char>& field, const uint8_t* src, size_t len, const char* label = nullptr) {
        field.alloc(len + 1);
        memcpy(field.get(), src, len);
        field[len] = '\0';
        latinToUTF8(field);
        if (field.strlen() > 0 && label && audio_id3data) {
            field.insert(label, 0);
            audio_id3data(field.get());
        }
    };

    if (InBuff.bufferFilled() == 128 && startsWith((const char*)p, "TAG")) {
        ps_ptr<char> title, artist, album, year, comment;

        readID3Field(title,   p + 3,     30, "Title: ");
        readID3Field(artist,  p + 33,    30, "Artist: ");
        readID3Field(album,   p + 63,    30, "Album: ");
        readID3Field(year,    p + 93,    4,  "Year: ");
        readID3Field(comment, p + 97,    30, "Comment: ");

        uint8_t zeroByte = p[125];
        uint8_t track = p[126];
        uint8_t genre8 = p[127];

        AUDIO_INFO(zeroByte ? "ID3 version: 1" : "ID3 Version 1.1");

        if (zeroByte == 0) {
            sprintf(chBuff.get(), "Track Number: %d", track);
            if (audio_id3data) audio_id3data(chBuff.get());
        }

        if (genre8 < 192) {
            sprintf(chBuff.get(), "Genre: %d", genre8);
            if (audio_id3data) audio_id3data(chBuff.get());
        }

        return true;
    }

    if (InBuff.bufferFilled() == 227 && startsWith((const char*)p, "TAG+")) { // "TAG+" "can exist as an extension, does not overwrite" TAG"
        AUDIO_INFO("ID3 version: 1 - Enhanced TAG");

        ps_ptr<char> title, artist, album, genre;

        readID3Field(title,  p + 4,     60, "Title: ");
        readID3Field(artist, p + 64,    60, "Artist: ");
        readID3Field(album,  p + 124,   60, "Album: ");
        readID3Field(genre,  p + 185,   30, "Genre: ");

        // optional expansion: speed, start-time, end-time
        return true;
    }

    return false;
    // [1] https://en.wikipedia.org/wiki/List_of_ID3v1_Genres
    // [2] https://en.wikipedia.org/wiki/ID3#ID3v1_and_ID3v1.1[5]
}
//##################################################################
boolean Audio::streamDetection(uint32_t bytesAvail) {
    if(!m_lastHost.valid()) {AUDIO_ERROR("m_lastHost is empty"); return false;}

    // if within one second the content of the audio buffer falls below the size of an audio frame 100 times,
    // issue a message
    if(m_sdet.tmr_slow + 1000 < millis()) {
        m_sdet.tmr_slow = millis();
        if(m_sdet.cnt_slow > 100) AUDIO_INFO("slow stream, dropouts are possible");
        m_sdet.cnt_slow = 0;
    }
    if(InBuff.bufferFilled() < InBuff.getMaxBlockSize()) m_sdet.cnt_slow++;
    if(bytesAvail) {
        m_sdet.tmr_lost = millis() + 1000;
        m_sdet.cnt_lost = 0;
    }
    if(InBuff.bufferFilled() > InBuff.getMaxBlockSize() * 2) return false; // enough data available to play

    // if no audio data is received within three seconds, a new connection attempt is started.
    if(m_sdet.tmr_lost < millis()) {
        m_sdet.cnt_lost++;
        m_sdet.tmr_lost = millis() + 1000;
        if(m_sdet.cnt_lost == 5) { // 5s no data?
            m_sdet.cnt_lost = 0;
            AUDIO_INFO("Stream lost -> try new connection");
            m_f_reset_m3u8Codec = false;
            httpPrint(m_lastHost.get());
            return true;
        }
    }
    return false;
}
//##################################################################
uint32_t Audio::find_m4a_atom(uint32_t fileSize, const char* atomName, uint32_t depth) {

    while (m_audiofile.position() < fileSize) {
        uint32_t atomStart = m_audiofile.position(); // Position of the current atom
        uint32_t atomSize;
        char atomType[5] = {0};
        m_audiofile.read((uint8_t*)&atomSize, 4);    // Read the atom size (4 bytes) and the atom type (4 bytes)
        if(!atomSize) m_audiofile.read((uint8_t*)&atomSize, 4); // skip 4 byte offset field
        m_audiofile.read((uint8_t*)atomType, 4);

        atomSize = bswap32(atomSize);              // Convert atom size from big-endian to little-endian
        ///    log_w("%*sAtom '%s' found at position %u with size %u bytes", depth * 2, "", atomType, atomStart, atomSize);
        if (strncmp(atomType, atomName, 4) == 0) return atomStart;

        if (atomSize == 1) {                       // If the atom has a size of 1, an 'Extended Size' is used
            uint64_t extendedSize;
            m_audiofile.read((uint8_t*)&extendedSize, 8);
            extendedSize = bswap64(extendedSize);
        //    log_w("%*sExtended size: %llu bytes\n", depth * 2, "", extendedSize);
            atomSize = (uint32_t)extendedSize;     // Limit to uint32_t for further processing
        }

        // If the atom is a container, read the atoms contained in it recursively
        if (strncmp(atomType, "moov", 4) == 0 || strncmp(atomType, "trak", 4) == 0 ||
            strncmp(atomType, "mdia", 4) == 0 || strncmp(atomType, "minf", 4) == 0 ||
            strncmp(atomType, "stbl", 4) == 0 || strncmp(atomType, "meta", 4) == 0 ||
            strncmp(atomType, "udta", 4) == 0 )  {
            // Go recursively into the atom, read the contents
            uint32_t pos = find_m4a_atom(atomStart + atomSize,  atomName, depth + 1);
            if(pos) return pos;
        } else {
            m_audiofile.seek(atomStart + atomSize);    // No container atom, jump to the next atom
        }
    }
    return 0;
}
//##################################################################
void Audio::seek_m4a_ilst() {    // ilist - item list atom, contains the metadata
    if(!m_audiofile) return; // guard
    m_audiofile.seek(0);
    uint32_t fileSize = m_audiofile.size();
    const char atomName[] = "ilst";
    uint32_t atomStart = find_m4a_atom(fileSize, atomName);
    if(!atomStart) {
        AUDIO_INFO("ma4 atom ilst not found");
        m_audiofile.seek(0);
        return;
    }

    uint32_t   seekpos = atomStart;
    const char info[12][6] = {"nam\0", "ART\0", "alb\0", "too\0", "cmt\0", "wrt\0", "tmpo\0", "trkn\0", "day\0", "cpil\0", "aART\0", "gen\0"};

    seekpos = atomStart;
    char buff[4];
    uint32_t len = 0;
    m_audiofile.seek(seekpos);
    m_audiofile.readBytes(buff, 4);
    len = bigEndian((uint8_t*)buff, 4);
    if(!len) {
        m_audiofile.readBytes(buff, 4); // 4bytes offset filed
        len = bigEndian((uint8_t*)buff, 4) + 16;
    }
    if(len > 1024) len = 1024;
    //    log_w("found at pos %i, len %i", seekpos, len);

    ps_ptr<uint8_t> data;
    data.alloc(len, "data");
    data.clear();
    len -= 4;
    m_audiofile.seek(seekpos);
    m_audiofile.read(data.get(), len);

    int offset = 0;
    for(int i = 0; i < 12; i++) {
        offset = specialIndexOf(data.get(), info[i], len, true); // seek info[] with '\0'
        if(offset > 0) {
            offset += 19;
            if(*(data.get() + offset) == 0) offset++;
            char   value[256] = {0};
            size_t temp = strlen((const char*)data.get() + offset);
            if(temp > 254) temp = 254;
            memcpy(value, (data.get() + offset), temp);
            value[temp] = '\0';
            ps_ptr<char>chBuf = {};
            if(i == 0)  chBuf.appendf("Title: %s", value);
            if(i == 1)  chBuf.appendf("Artist: %s", value);
            if(i == 2)  chBuf.appendf("Album: %s", value);
            if(i == 3)  chBuf.appendf("Encoder: %s", value);
            if(i == 4)  chBuf.appendf("Comment: %s", value);
            if(i == 5)  chBuf.appendf("Composer: %s", value);
            if(i == 6)  chBuf.appendf("BPM: %s", value);
            if(i == 7)  chBuf.appendf("Track Number: %s", value);
            if(i == 8)  chBuf.appendf("Year: %s", value);
            if(i == 9)  chBuf.appendf("Compile: %s", value);
            if(i == 10) chBuf.appendf("Album Artist: %s", value);
            if(i == 11) chBuf.appendf("Types of: %s", value);
            if(chBuf.valid()) {
                if(audio_id3data) audio_id3data(chBuf.get());
            }
        }
    }
    m_f_m4aID3dataAreRead = true;
    m_audiofile.seek(0);
    return;
}
//##################################################################
void Audio::seek_m4a_stsz() {
    // stsz says what size each sample is in bytes. This is important for the decoder to be able to start at a chunk,
    // and then go through each sample by its size. The stsz atom can be behind the audio block. Therefore, searching
    // for the stsz atom is only applicable to local files.

    /* atom hierarchy (example)_________________________________________________________________________________________

      ftyp -> moov -> trak -> tkhd
              free    udta    mdia -> mdhd
              mdat                    hdlr
              mvhd                    minf -> smhd
                                              dinf
                                              stbl -> stsd
                                                      stts
                                                      stsc
                                                      stsz -> determine and return the position and number of entries
                                                      stco
      __________________________________________________________________________________________________________________*/

    struct m4a_Atom {
        int  pos;
        int  size;
        char name[5] = {0};
    } atom, at, tmp;

    // c99 has no inner functions, lambdas are only allowed from c11, please don't use ancient compiler
    auto atomItems = [&](uint32_t startPos) { // lambda, inner function
        char temp[5] = {0};
        m_audiofile.seek(startPos);
        m_audiofile.readBytes(temp, 4);
        atom.size = bigEndian((uint8_t*)temp, 4);
        if(!atom.size) atom.size = 4; // has no data, length is 0
        m_audiofile.readBytes(atom.name, 4);
        atom.name[4] = '\0';
        atom.pos = startPos;
        return atom;
    };
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    uint32_t stsdPos = 0;
    uint16_t stsdSize = 0;
    boolean  found = false;
    uint32_t seekpos = 0;
    uint32_t filesize = getFileSize();
    char     name[6][5] = {"moov", "trak", "mdia", "minf", "stbl", "stsz"};
    char     noe[4] = {0};

    if(!m_audiofile) return; // guard

    at.pos = 0;
    at.size = filesize;
    seekpos = 0;

    for(int i = 0; i < 6; i++) {
        found = false;
        while(seekpos < at.pos + at.size) {
            tmp = atomItems(seekpos);
            seekpos += tmp.size;
        //  log_i("tmp.name %s, tmp.size %i, seekpos %i", tmp.name, tmp.size, seekpos);
            if(strcmp(tmp.name, name[i]) == 0) {
                memcpy((void*)&at, (void*)&tmp, sizeof(tmp));
                found = true;
            }
            // log_i("name %s pos %d, size %d", tmp.name, tmp.pos, tmp.size);
            if(strcmp(tmp.name, "stsd") == 0) { // in stsd we can found mp4a atom that contains the audioitems
                stsdPos = tmp.pos;
                stsdSize = tmp.size;
            }
        }
        if(!found) goto noSuccess;
        seekpos = at.pos + 8; // 4 bytes size + 4 bytes name
    }
    seekpos += 8; // 1 byte version + 3 bytes flags + 4  bytes sample size
    m_audiofile.seek(seekpos);
    m_audiofile.readBytes(noe, 4); // number of entries
    m_stsz_numEntries = bigEndian((uint8_t*)noe, 4);
    // log_i("number of entries in stsz: %d", m_stsz_numEntries);
    m_stsz_position = seekpos + 4;
    if(stsdSize) {
        m_audiofile.seek(stsdPos);
        uint8_t data[128];
        m_audiofile.readBytes((char*)data, 128);
        int offset = specialIndexOf(data, "mp4a", stsdSize);
        if(offset > 0) {
            int channel = bigEndian(data + offset + 20, 2); // audio parameter must be set before starting
            int bps = bigEndian(data + offset + 22, 2);     // the aac decoder. There are RAW blocks only in m4a
            int srate = bigEndian(data + offset + 26, 4);   //
//            setBitsPerSample(bps);
//            setChannels(channel);
//            setSampleRate(srate);
//            setBitrate(bps * channel * srate);
	       m_bitRate = bps * channel * srate;
            AUDIO_INFO("ch: %i, bps: %i, sr: %i", channel, bps, srate);
        }
    }
    m_audiofile.seek(0);
    return;

noSuccess:
    m_stsz_numEntries = 0;
    m_stsz_position = 0;
    AUDIO_ERROR("m4a atom stsz not found");
    m_audiofile.seek(0);
    return;
}
//##################################################################
uint32_t Audio::m4a_correctResumeFilePos() {
    // In order to jump within an m4a file, the exact beginning of an aac block must be found. Since m4a cannot be
    // streamed, i.e. there is no syncword, an imprecise jump can lead to a crash.

    if(!m_stsz_position) return m_audioDataStart; // guard

    typedef union {
        uint8_t  u8[4];
        uint32_t u32;
    } tu;
    tu uu;

    uint32_t i = 0, pos = m_audioDataStart;
    uint32_t filePtr = m_audiofile.position();
    bool found = false;
    m_audiofile.seek(m_stsz_position);

    while(i < m_stsz_numEntries) {
        i++;
        uu.u8[3] = m_audiofile.read();
        uu.u8[2] = m_audiofile.read();
        uu.u8[1] = m_audiofile.read();
        uu.u8[0] = m_audiofile.read();
        pos += uu.u32;
        if(pos >= m_resumeFilePos) {found = true; break;}
    }
    if(!found)  return -1; // not found

    m_audiofile.seek(filePtr); // restore file pointer
    return pos - m_resumeFilePos; // return the number of bytes to jump
}
//##################################################################
uint32_t Audio::ogg_correctResumeFilePos() {
    // The starting point is the next OggS magic word
    vTaskDelay(1);
    // // log_w("in_resumeFilePos %i", resumeFilePos);

    auto find_sync_word = [&](uint8_t* pos, uint32_t av) -> int {
        int steps = 0;
        while(av--) {
            if(pos[steps] == 'O'){
                if(pos[steps + 1] == 'g') {
                    if(pos[steps + 2] == 'g') {
                        if(pos[steps + 3] == 'S') { // Check for the second part of magic word
                            return steps;   // Magic word found, return the number of steps
                        }
                    }
                }
            }
            steps++;
        }
        return -1; // Return -1 if OggS magic word is not found
    };

    uint8_t*       readPtr = InBuff.getReadPtr();
    size_t         av = InBuff.getMaxAvailableBytes();
    int32_t        steps = 0;

    if(av < InBuff.getMaxBlockSize()) return -1; // guard

    steps = find_sync_word(readPtr, av);
    if(steps == -1) return -1;
    return steps; // Return the number of steps to the sync word
}
//##################################################################
int32_t Audio::flac_correctResumeFilePos() {

    auto find_sync_word = [&](uint8_t* pos, uint32_t av) -> int {
        int steps = 0;
        while(av--) {
            char c = pos[steps];
            steps++;
            if(c == 0xFF) { // First byte of sync word found
                char nextByte = pos[steps];
                steps++;
                if(nextByte == 0xF8) {     // Check for the second part of sync word
                    return steps - 2;       // Sync word found, return the number of steps
                }
            }
        }
        return -1; // Return -1 if sync word is not found
    };

    uint8_t*       readPtr = InBuff.getReadPtr();
    size_t         av = InBuff.getMaxAvailableBytes();
    int32_t        steps = 0;

    if(av < InBuff.getMaxBlockSize()) return -1; // guard

    steps = find_sync_word(readPtr, av);
    if(steps == -1) return -1;
    return steps; // Return the number of steps to the sync word
}
//##################################################################
int32_t Audio::mp3_correctResumeFilePos() {

    int32_t        steps = 0;
    int32_t        sumSteps = 0;
    uint8_t*       pos = InBuff.getReadPtr();
    size_t         av = InBuff.getMaxAvailableBytes();

    if(av < InBuff.getMaxBlockSize()) return -1; // guard

    while(true) {
        steps = MP3FindSyncWord(pos, av);
        if(steps == 0)break;
        if(steps == -1) return -1;
        pos += steps;
        sumSteps += steps;
    }
    // log_w("found sync word at %i  sync1 = 0x%02X, sync2 = 0x%02X", readPtr - pos, *readPtr, *(readPtr + 1));
    return sumSteps; // return the position of the first byte of the frame
}
//###################################################################
//###################################################################
//###################################################################
//###################################################################
uint8_t Audio::determineOggCodec(uint8_t* data, uint16_t len){
    // if we have contentType == application/ogg; codec cn be OPUS, FLAC or VORBIS
    // let's have a look, what it is
    int idx = specialIndexOf(data, "OggS", 6);
    if(idx != 0){
        if(specialIndexOf(data, "fLaC", 6)) {
//           oggHead_idx = idx;
//           oggHead_data = data;
//           parseFlacComment(data, len);
            read_FLAC_Header(data, len);
           return CODEC_FLAC;
        }
        return CODEC_NONE;
    }
    data += 27;
    idx = specialIndexOf(data, "OpusHead", 40);
    if(idx >= 0) {
//       oggHead_idx = idx;
//       oggHead_data = data;
//       parseOpusComment(data, len);
       return CODEC_OPUS;
       }
    idx = specialIndexOf(data, "fLaC", 40);
    if(idx >= 0) {
//       oggHead_idx = idx;
//       oggHead_data = data;
//       parseFlacComment(data, len);
        read_FLAC_Header(data, len);
       return CODEC_FLAC;
       }
    idx = specialIndexOf(data, "vorbis", 40);
    if(idx >= 0){
//       oggHead_idx = idx;
//       oggHead_data = data;
//       parseVorbisComment(data, len);
        return CODEC_VORBIS;
        }
    return CODEC_NONE;
}
/* //###################################################################
int32_t Audio::OGG_specialIndexOf(uint8_t* base, const char* str, int32_t baselen, bool exact){
    int32_t result = 0;  					// seek for str in buffer or in header up to baselen, not nullterninated
    if (strlen(str) > baselen) return -1; 			// if exact == true seekstr in buffer must have "\0" at the end
    for (int32_t i = 0; i < baselen - strlen(str); i++){
        result = i;
        for (int32_t j = 0; j < strlen(str) + exact; j++){
            if (*(base + i + j) != *(str + j)){
                result = -1;
                break;
            }
        }
        if (result >= 0) break;
    }
    return result;
}
//################################################################### */
char* Audio::VORBISgetStreamTitle(){
    if(s_f_vorbisNewSteamTitle){
        s_f_vorbisNewSteamTitle = false;
        return s_vorbisChbuf;
    }
    return NULL;
}
//###################################################################
int8_t Audio::parseVorbisComment(uint8_t *inbuf, int16_t nBytes){      // reference https://xiph.org/vorbis/doc/v-comment.html

    int8_t   ret = 0;

    // first bytes are: '.vorbis'
    uint32_t pos = 7;
    uint32_t vendorLength       = *(inbuf + pos + 3) << 24; // lengt of vendor string, e.g. Xiph.Org libVorbis I 20070622
             vendorLength      += *(inbuf + pos + 2) << 16;
             vendorLength      += *(inbuf + pos + 1) << 8;
             vendorLength      += *(inbuf + pos);
    uint16_t s_vorbisCommentHeaderLength = 0;
    int32_t   s_commentLength = 0;

    if(vendorLength > 254){  // guard
       log_e("vorbis comment too long, vendorLength %i", vendorLength);
       return 0;
    }

    memcpy(s_vorbisChbuf, inbuf + 11, vendorLength);
    s_vorbisChbuf[vendorLength] = '\0';
    pos += 4 + vendorLength;
    s_vorbisCommentHeaderLength -= (7 + 4 + vendorLength);

    // log_i("vendorLength %x", vendorLength);
    // log_i("vendorString %s", s_vorbisChbuf);

    uint8_t nrOfComments = *(inbuf + pos);
    // log_i("nrOfComments %i", nrOfComments);
    pos += 4;
    s_vorbisCommentHeaderLength -= 4;

    int32_t idx = 0;
    char* artist = NULL;
    char* title  = NULL;
    uint32_t commentLength = 0;

    for(int32_t i = 0; i < nrOfComments; i++){
        commentLength  = 0;
        commentLength  = *(inbuf + pos + 3) << 24;
        commentLength += *(inbuf + pos + 2) << 16;
        commentLength += *(inbuf + pos + 1) << 8;
        commentLength += *(inbuf + pos);
        s_commentLength = commentLength;

        uint8_t cl = min((uint32_t)254, commentLength);
        memcpy(s_vorbisChbuf, inbuf + pos +  4, cl);
        s_vorbisChbuf[cl] = '\0';

        // log_i("commentLength %i comment %s", commentLength, s_vorbisChbuf);

        idx =  specialIndexOf((uint8_t*)s_vorbisChbuf, "artist=", 10);
        if(idx != 0) idx =  specialIndexOf((uint8_t*)s_vorbisChbuf, "ARTIST=", 10);
        if(idx == 0){ artist = strndup((const char*)(s_vorbisChbuf + 7), commentLength - 7); s_commentLength = 0;
        AUDIO_INFO("ARTIST: %s", artist); }

        idx =  specialIndexOf((uint8_t*)s_vorbisChbuf, "title=", 10);
        if(idx != 0) idx =  specialIndexOf((uint8_t*)s_vorbisChbuf, "TITLE=", 10);
        if(idx == 0){ title = strndup((const char*)(s_vorbisChbuf + 6), commentLength - 6); s_commentLength = 0;
        AUDIO_INFO("TITLE: %s", title); }
    }

    if(artist && title){
        strcpy(s_vorbisChbuf, artist);
        strcat(s_vorbisChbuf, " - ");
        strcat(s_vorbisChbuf, title);
        s_f_vorbisNewSteamTitle = true;
        ret = 1;
    }
    else if(artist){
        strcpy(s_vorbisChbuf, artist);
        s_f_vorbisNewSteamTitle = true;
        ret = 1;
    }
    else if(title){
        strcpy(s_vorbisChbuf, title);
        s_f_vorbisNewSteamTitle = true;
        ret = 1;
    }
    if(artist){free(artist); artist = NULL;}
    if(title) {free(title);  title = NULL;}

    return ret;
}
//###################################################################
char* Audio::OPUSgetStreamTitle(){
    if(s_f_opusNewSteamTitle){
        s_f_opusNewSteamTitle = false;
        return s_opusSteamTitle;
    }
    return NULL;
}
//###################################################################
int8_t Audio::parseOpusComment(uint8_t *inbuf, int32_t nBytes){      // reference https://exiftool.org/TagNames/Vorbis.html#Comments
                                                       // reference https://www.rfc-editor.org/rfc/rfc7845#section-5

    int32_t idx = specialIndexOf(inbuf, "OpusTags", 10);
    if(idx != 0) return 0; // is not OpusTags

    int8_t   ret = 0;

    char* artist = NULL;
    char* title  = NULL;

    uint16_t pos = 8;
             nBytes -= 8;
    uint32_t vendorLength       = *(inbuf + 11) << 24; // lengt of vendor string, e.g. Lavf58.65.101
             vendorLength      += *(inbuf + 10) << 16;
             vendorLength      += *(inbuf +  9) << 8;
             vendorLength      += *(inbuf +  8);
    pos += vendorLength + 4;
    nBytes -= (vendorLength + 4);
    uint32_t commentListLength  = *(inbuf + 3 + pos) << 24; // nr. of comment entries
             commentListLength += *(inbuf + 2 + pos) << 16;
             commentListLength += *(inbuf + 1 + pos) << 8;
             commentListLength += *(inbuf + 0 + pos);
    pos += 4;
    nBytes -= 4;

    for(int32_t i = 0; i < commentListLength; i++){
        uint32_t commentStringLen   = *(inbuf + 3 + pos) << 24;
                 commentStringLen  += *(inbuf + 2 + pos) << 16;
                 commentStringLen  += *(inbuf + 1 + pos) << 8;
                 commentStringLen  += *(inbuf + 0 + pos);
        pos += 4;
        nBytes -= 4;
        idx = specialIndexOf(inbuf + pos, "artist=", 10);
        if(idx != 0) idx = specialIndexOf(inbuf + pos, "ARTIST=", 10);
        if(idx == 0){ artist = strndup((const char*)(inbuf + pos + 7), commentStringLen - 7);
        AUDIO_INFO("ARTIST: %s", artist);
        }
        idx = specialIndexOf(inbuf + pos, "title=", 10);
        if(idx != 0) idx = specialIndexOf(inbuf + pos, "TITLE=", 10);
        if(idx == 0){ title = strndup((const char*)(inbuf + pos + 6), commentStringLen - 6);
        AUDIO_INFO("TITLE: %s", title);
        }
    }

    if(artist && title){
        strcpy(s_opusSteamTitle, artist);
        strcat(s_opusSteamTitle, " - ");
        strcat(s_opusSteamTitle, title);
        s_f_opusNewSteamTitle = true;
        ret = 1;
    }
    else if(artist){
        strcpy(s_opusSteamTitle, artist);
        s_f_opusNewSteamTitle = true;
        ret = 1;
    }
    else if(title){
        strcpy(s_opusSteamTitle, title);
        s_f_opusNewSteamTitle = true;
        ret = 1;
    }
    if(artist){free(artist); artist = NULL;}
    if(title) {free(title);  title = NULL;}

    return ret;
}
//###################################################################
char* Audio::FLACgetStreamTitle(){
    if(s_f_flacNewStreamtitle){
        s_f_flacNewStreamtitle = false;
        return s_flacStreamTitle;
    }
    return NULL;
}
//###################################################################
int8_t Audio::parseFlacComment(uint8_t *inbuf, int16_t nBytes){

    int8_t   ret = 0;
    uint16_t pos = 0;
    int32_t  blockLength = 0;
    uint32_t vendorLength = 0;
    uint32_t commemtStringLength = 0;
    uint32_t userCommentListLength = 0;
    uint8_t  mdBlockHeader = 0;
    uint8_t  blockType = 0;
    uint16_t s_f_lastMetaDataBlock = 0;
    uint16_t s_flacRemainBlockPicLen = 0;
    char*    vb[2] = {0}; 					// vorbis comment

//    enum {streamInfo, padding, application, seekTable, vorbisComment, cueSheet, picture};

//    while(true){
        mdBlockHeader         = *(inbuf + pos);
        s_f_lastMetaDataBlock = mdBlockHeader & 0b10000000; 		//log_w("lastMdBlockFlag %i", s_f_lastMetaDataBlock);
        blockType             = mdBlockHeader & 0b01111111; 			//log_w("blockType %i", blockType);

        blockLength        = *(inbuf + pos + 1) << 16;
        blockLength       += *(inbuf + pos + 2) << 8;
        blockLength       += *(inbuf + pos + 3); 					//log_w("blockLength %i", blockLength);

        nBytes -= 4;
        pos += 4;

        switch(blockType) {

            case 0:
                //log_i("nBytes %i, blockLength %i", nBytes, blockLength);
                pos += blockLength;
                nBytes -= blockLength;
                break;

            case 4: 								// https://www.xiph.org/vorbis/doc/v-comment.html
                vendorLength  = *(inbuf + pos + 3) << 24;
                vendorLength += *(inbuf + pos + 2) << 16;
                vendorLength += *(inbuf + pos + 1) <<  8;
                vendorLength += *(inbuf + pos + 0);
                if(vendorLength > 1024){  log_e("vendorLength > 1024 bytes");  }

                pos += 4 + vendorLength;
                userCommentListLength  = *(inbuf + pos + 3) << 24;
                userCommentListLength += *(inbuf + pos + 2) << 16;
                userCommentListLength += *(inbuf + pos + 1) <<  8;
                userCommentListLength += *(inbuf + pos + 0);

                pos += 4;
                commemtStringLength = 0;

                for(int32_t i = 0; i < userCommentListLength; i++){
                    commemtStringLength  = *(inbuf + pos + 3) << 24;
                    commemtStringLength += *(inbuf + pos + 2) << 16;
                    commemtStringLength += *(inbuf + pos + 1) <<  8;
                    commemtStringLength += *(inbuf + pos + 0);

                    if((specialIndexOf(inbuf + pos + 4, "TITLE", 6) == 0) || (specialIndexOf(inbuf + pos + 4, "title", 6) == 0)){
                        vb[0] = strndup((const char*)(inbuf + pos + 4 + 6), min((uint32_t)127, commemtStringLength - 6));
                        AUDIO_INFO("TITLE: %s", vb[0]);
                    }
                    if((specialIndexOf(inbuf + pos + 4, "ARTIST", 7) == 0) || (specialIndexOf(inbuf + pos + 4, "artist", 7) == 0)){
                        vb[1] = strndup((const char*)(inbuf + pos + 4 + 7), min((uint32_t)127, commemtStringLength - 7));
                        AUDIO_INFO("ARTIST: %s", vb[1]);
                    }
                    if((specialIndexOf(inbuf + pos + 4, "ALBUM", 6) == 0) || (specialIndexOf(inbuf + pos + 4, "album", 6) == 0)){
                        vb[2] = strndup((const char*)(inbuf + pos + 4 + 6), min((uint32_t)127, commemtStringLength - 6));
                        AUDIO_INFO("ALBUM: %s", vb[2]);
                    }
                }

                memset(s_flacStreamTitle, 0, 256);
                if(vb[1] && vb[0]){ 				// artist and title
                    strcpy(s_flacStreamTitle, vb[1]);
                    strcat(s_flacStreamTitle, " - ");
                    strcat(s_flacStreamTitle, vb[0]);
                    s_f_flacNewStreamtitle = true;
                    ret = 1;
                }
                else if(vb[1]){ 					// only artist
                    strcpy(s_flacStreamTitle, vb[1]);
                    s_f_flacNewStreamtitle = true;
                    ret = 1;
                }
                else if(vb[0]){ 					// only title
                    strcpy(s_flacStreamTitle, vb[0]);
                    s_f_flacNewStreamtitle = true;
                    ret = 1;
                }
                for(int32_t i = 0; i < 8; i++){
                    if(vb[i]){free(vb[i]); vb[i] = NULL;}
                }
                break;
        }
//    }
    return ret;
}
//###################################################################
// separate task for decoding and outputting the data. 'playAudioData()' is started periodically and fetches the data from the InBuffer.
//  This ensures that the I2S-DMA is always sufficiently filled, even if the Arduino 'loop' is stuck.
//****************************************************************************************
void Audio::setAudioTaskCore(uint8_t coreID){  // Recommendation:If the ARDUINO RUNNING CORE is 1, the audio task should be core 0 or vice versa
    if(coreID > 1) return;
    stopAudioTask();
    xSemaphoreTake(mutex_audioTask, 0.3 * configTICK_RATE_HZ);
    m_audioTaskCoreId = coreID;
    xSemaphoreGive(mutex_audioTask);
    startAudioTask();
}
//****************************************************************************************
void Audio::startAudioTask() {
    if (m_f_audioTaskIsRunning) {
        log_i("Task is already running.");
        return;
    }
    m_f_audioTaskIsRunning = true;


    m_audioTaskHandle = xTaskCreateStaticPinnedToCore(
        &Audio::taskWrapper,    /* Function to implement the task */
        "PeriodicTask",         /* Name of the task */
        AUDIO_STACK_SIZE,       /* Stack size in words */
        this,                   /* Task input parameter */
        2,                      /* Priority of the task */
        xAudioStack,            /* Task stack */
        &xAudioTaskBuffer,      /* Memory for the task's control block */
        m_audioTaskCoreId       /* Core where the task should run */
    );
}
//****************************************************************************************
void Audio::stopAudioTask()  {
    if (!m_f_audioTaskIsRunning) {
        log_i("audio task is not running.");
        return;
    }
    xSemaphoreTake(mutex_audioTask, 0.3 * configTICK_RATE_HZ);
    m_f_audioTaskIsRunning = false;
    if (m_audioTaskHandle != nullptr) {
        vTaskDelete(m_audioTaskHandle);
        m_audioTaskHandle = nullptr;
    }
    xSemaphoreGive(mutex_audioTask);
}
//****************************************************************************************
void Audio::taskWrapper(void *param) {
    Audio *runner = static_cast<Audio*>(param);
    runner->audioTask();
}
//****************************************************************************************
void Audio::audioTask() {
    while (m_f_audioTaskIsRunning) {
        vTaskDelay(1 / portTICK_PERIOD_MS);  // periodically every x ms
        performAudioTask();
    }
    vTaskDelete(nullptr);  				// Delete this task
}
//****************************************************************************************
void Audio::performAudioTask() {
    if(!m_f_running) return;
    if(!m_f_stream) return;
    if(m_codec == CODEC_NONE) return; // wait for codec is  set
    if(m_codec == CODEC_OGG)  return; // wait for FLAC, VORBIS or OPUS
    xSemaphoreTake(mutex_audioTask, 0.3 * configTICK_RATE_HZ);
//    while(m_validSamples) { vTaskDelay(20 / portTICK_PERIOD_MS); playChunk();} // I2S buffer full
    while(m_validSamples) { vTaskDelay(20 / portTICK_PERIOD_MS); } // I2S buffer full
    playAudioData();
    xSemaphoreGive(mutex_audioTask);
}
//****************************************************************************************
uint32_t Audio::getHighWatermark(){
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(m_audioTaskHandle);
    return highWaterMark; // dwords
}
//****************************************************************************************
#endif  //  if I2S_DOUT==255
