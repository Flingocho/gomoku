# 🎵 Audio Files for Gomoku

## Audio System Overview

This folder contains all audio assets for the Gomoku game. The audio system uses SFML Audio module and supports **OGG Vorbis format only** (SFML does not support MP3 natively).

## 📁 Current Audio Files

### Music (Streamed)
- **`main_theme.ogg`** - Main background music (loops continuously during gameplay)
  - Set to 50% volume by default
  - Streams from disk to save memory

### Sound Effects (In Memory)
- **`place_piece.ogg`** - Plays when a valid piece is placed on the board
- **`invalid_move.ogg`** - Plays when an invalid move is attempted
- **`click_menu.ogg`** - UI click sound for menu button interactions
- **`victory.ogg`** - Victory fanfare when player wins
- **`defeat.ogg`** - Sound when player loses to AI

## 🎮 Audio Controls

- **Default Volume**: 70% for sound effects, 50% for music
- **Mute Toggle**: Press 'M' key during gameplay (feature available in code)
- **No Audio Mode**: Game runs normally even if audio files are missing (with console warnings)

## 🔧 Technical Specifications

### Format Requirements
- **Container**: OGG Vorbis (.ogg)
- **Codec**: Vorbis audio compression
- **Why OGG?**: SFML's built-in audio module natively supports OGG but not MP3

### Recommended Settings
- **Music Files**:
  - Bitrate: 128-192 kbps
  - Sample Rate: 44100 Hz
  - Channels: Stereo (2)
  
- **Sound Effects**:
  - Bitrate: 96-128 kbps  
  - Sample Rate: 44100 Hz
  - Channels: Mono (1) or Stereo (2)
  - Duration: 0.5-2 seconds (short and punchy)

## 🔄 Converting MP3 to OGG

If you have MP3 files, you can convert them to OGG format:

### Using ffmpeg (Recommended)
```bash
# Install ffmpeg (Ubuntu/Debian)
sudo apt-get install ffmpeg

# Convert with good quality
ffmpeg -i input.mp3 -c:a libvorbis -q:a 4 output.ogg

# Batch convert all MP3 files in folder
for file in *.mp3; do ffmpeg -i "$file" -c:a libvorbis -q:a 4 "${file%.mp3}.ogg"; done
```

### Using Audacity
1. Open MP3 file in Audacity
2. File → Export → Export as OGG Vorbis
3. Set quality to 5-7 for good balance

### Online Converters
- https://cloudconvert.com/mp3-to-ogg
- https://convertio.co/mp3-ogg/
- https://online-audio-converter.com/

## 🎨 Free Audio Resources

### Sound Effects
- **Freesound**: https://freesound.org/ (CC licensed, community sounds)
- **OpenGameArt**: https://opengameart.org/ (game-specific audio)
- **Zapsplat**: https://www.zapsplat.com/ (free for games)
- **Kenney**: https://kenney.nl/assets (game audio packs)

### Music
- **Incompetech**: https://incompetech.com/ (royalty-free music by Kevin MacLeod)
- **FreePD**: https://freepd.com/ (public domain music)
- **Bensound**: https://www.bensound.com/ (free music with attribution)
- **Purple Planet**: https://www.purple-planet.com/ (royalty-free music)

### Tips for Choosing Audio
- **Piece Placement**: Short, soft "tap" or "click" sound
- **Invalid Move**: Subtle "error" beep or "buzzer" 
- **Menu Click**: Clean UI click/button press
- **Victory**: Triumphant fanfare (3-5 seconds)
- **Defeat**: Somber or descending tone (2-3 seconds)
- **Background Music**: Calm, non-intrusive instrumental (2-5 minutes, loops well)

## 🛠️ Implementation Details

### C++ Code Structure
```cpp
// AudioManager class (src/audio_manager.cpp)
class AudioManager {
    sf::Music music;                    // Streamed music
    std::map<std::string, sf::SoundBuffer> soundBuffers;
    std::map<std::string, sf::Sound> sounds;
    
    void loadMusic(const std::string& filepath);
    void loadSound(const std::string& name, const std::string& filepath);
    void playSound(const std::string& name);
};
```

### Audio Loading
All audio files are loaded at game startup from the `sounds/` directory:
- Music is streamed (not loaded into memory)
- Sound effects are fully loaded into memory for instant playback
- Missing files trigger console warnings but don't crash the game

### Performance Considerations
- **Memory**: ~1-5 MB for all sound effects combined
- **CPU**: Minimal overhead (SFML handles decoding efficiently)
- **Disk I/O**: Music streaming uses ~150-200 KB/s

## ✅ Current Status

All audio files are present and functional:
- ✅ `main_theme.ogg` - Looping background music
- ✅ `place_piece.ogg` - Piece placement sound
- ✅ `invalid_move.ogg` - Invalid move feedback
- ✅ `click_menu.ogg` - Menu interaction
- ✅ `victory.ogg` - Win sound
- ✅ `defeat.ogg` - Loss sound

The game will run without audio files, but will print warnings to the console if any are missing.

## 🐛 Troubleshooting

### No Sound Playing
1. Check that audio files exist in `sounds/` folder
2. Verify files are in OGG format (not MP3)
3. Check console for error messages
4. Ensure system audio is not muted

### Audio Crackling/Stuttering
- Reduce audio bitrate (try 96 kbps for sound effects)
- Ensure OGG files are not corrupted
- Check system CPU usage

### Volume Too Low/High
- Adjust in `src/audio_manager.cpp`:
  ```cpp
  sound.setVolume(70.f);  // 0-100
  music.setVolume(50.f);
  ```
