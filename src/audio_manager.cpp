#include "../include/audio_manager.hpp"
#include <iostream>

AudioManager::AudioManager()
    : m_musicVolume(50.0f)
    , m_soundVolume(70.0f)
    , m_masterVolume(100.0f)
    , m_muted(false)
{
}

bool AudioManager::loadMusic(const std::string& filepath) {
    if (!m_music.openFromFile(filepath)) {
        std::cerr << "Failed to load music: " << filepath << std::endl;
        return false;
    }
    m_music.setVolume(m_musicVolume * m_masterVolume / 100.0f);
    std::cout << "Loaded music: " << filepath << std::endl;
    return true;
}

void AudioManager::playMusic(bool loop) {
    if (m_muted) return;
    m_music.setLoop(loop);
    m_music.play();
}

void AudioManager::pauseMusic() {
    m_music.pause();
}

void AudioManager::stopMusic() {
    m_music.stop();
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = std::max(0.0f, std::min(100.0f, volume));
    updateVolumes();
}

bool AudioManager::loadSound(const std::string& name, const std::string& filepath) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(filepath)) {
        std::cerr << "Failed to load sound: " << filepath << std::endl;
        return false;
    }
    
    m_soundBuffers[name] = buffer;
    m_sounds[name] = std::make_unique<sf::Sound>();
    m_sounds[name]->setBuffer(m_soundBuffers[name]);
    m_sounds[name]->setVolume(m_soundVolume * m_masterVolume / 100.0f);
    
    std::cout << "Loaded sound '" << name << "': " << filepath << std::endl;
    return true;
}

void AudioManager::playSound(const std::string& name) {
    if (m_muted) return;
    
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        // Stop if already playing, then play from start
        if (it->second->getStatus() == sf::Sound::Playing) {
            it->second->stop();
        }
        it->second->play();
    } else {
        std::cerr << "Sound '" << name << "' not found!" << std::endl;
    }
}

void AudioManager::setSoundVolume(float volume) {
    m_soundVolume = std::max(0.0f, std::min(100.0f, volume));
    updateVolumes();
}

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = std::max(0.0f, std::min(100.0f, volume));
    updateVolumes();
}

void AudioManager::toggleMute() {
    m_muted = !m_muted;
    if (m_muted) {
        m_music.setVolume(0);
        for (auto& [name, sound] : m_sounds) {
            sound->setVolume(0);
        }
    } else {
        updateVolumes();
    }
}

void AudioManager::updateVolumes() {
    if (m_muted) return;
    
    // Update music volume
    m_music.setVolume(m_musicVolume * m_masterVolume / 100.0f);
    
    // Update all sound effects volumes
    for (auto& [name, sound] : m_sounds) {
        sound->setVolume(m_soundVolume * m_masterVolume / 100.0f);
    }
}
