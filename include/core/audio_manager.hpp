#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <SFML/Audio.hpp>
#include <string>
#include <map>
#include <memory>

// AudioManager handles all music and sound effects for the game
// SFML supports: OGG, WAV, FLAC (NOT MP3 due to licensing)
class AudioManager {
public:
    AudioManager();
    ~AudioManager() = default;

    // Music controls (for background music - streamed from file)
    bool loadMusic(const std::string& filepath);
    void playMusic(bool loop = true);
    void pauseMusic();
    void stopMusic();
    void setMusicVolume(float volume); // 0-100

    // Sound effects (for short sounds - loaded in memory)
    bool loadSound(const std::string& name, const std::string& filepath);
    void playSound(const std::string& name);
    void setSoundVolume(float volume); // 0-100

    // Master volume control
    void setMasterVolume(float volume); // 0-100
    
    // Toggle audio on/off
    void toggleMute();
    bool isMuted() const { return m_muted; }

private:
    // Background music
    sf::Music m_music;
    float m_musicVolume;
    
    // Sound effects
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::map<std::string, std::unique_ptr<sf::Sound>> m_sounds;
    float m_soundVolume;
    
    // Master controls
    float m_masterVolume;
    bool m_muted;
    
    void updateVolumes();
};

#endif // AUDIO_MANAGER_HPP
