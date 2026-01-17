#!/bin/bash
# Scripts to install dependencies

# Create external directory
mkdir -p external/deps
cd external

# Install SFML if not present
if [ ! -d "sfml" ]; then
    echo "Downloading SFML..."
    wget -q https://www.sfml-dev.org/files/SFML-2.5.1-linux-gcc-64-bit.tar.gz
    tar -xzf SFML-2.5.1-linux-gcc-64-bit.tar.gz
    mv SFML-2.5.1 sfml
    rm SFML-2.5.1-linux-gcc-64-bit.tar.gz

    # Download dependencies
    cd deps
    echo "Downloading dependencies..."

    # List of debs to download
    DEBS=(
        "http://archive.ubuntu.com/ubuntu/pool/universe/o/openal-soft/libopenal1_1.19.1-1_amd64.deb"
        "http://archive.ubuntu.com/ubuntu/pool/main/libv/libvorbis/libvorbis0a_1.3.6-2ubuntu1_amd64.deb"
        "http://archive.ubuntu.com/ubuntu/pool/main/libv/libvorbis/libvorbisenc2_1.3.6-2ubuntu1_amd64.deb"
        "http://archive.ubuntu.com/ubuntu/pool/main/libv/libvorbis/libvorbisfile3_1.3.6-2ubuntu1_amd64.deb"
        "http://archive.ubuntu.com/ubuntu/pool/main/libo/libogg/libogg0_1.3.4-0ubuntu1_amd64.deb"
        "http://security.ubuntu.com/ubuntu/pool/main/f/flac/libflac8_1.3.3-1ubuntu0.2_amd64.deb"
        "http://archive.ubuntu.com/ubuntu/pool/universe/s/sndio/libsndio7.0_1.5.0-3_amd64.deb"
    )

    for url in "${DEBS[@]}"; do
        wget -q "$url"
        deb_name=$(basename "$url")
        ar x "$deb_name" data.tar.xz
        tar -xf data.tar.xz
        cp -n usr/lib/x86_64-linux-gnu/*.so* ../sfml/lib/
        rm "$deb_name" data.tar.xz
    done
    cd ..

    # Create symlinks
    cd sfml/lib
    ln -sf libopenal.so.1 libopenal.so
    ln -sf libvorbisenc.so.2 libvorbisenc.so
    ln -sf libvorbisfile.so.3 libvorbisfile.so
    ln -sf libvorbis.so.0 libvorbis.so
    ln -sf libogg.so.0 libogg.so
    ln -sf libFLAC.so.8 libFLAC.so
    ln -sf libsndio.so.7.0 libsndio.so

    ln -sf libsfml-graphics.so.2.5.1 libsfml-graphics.so
    ln -sf libsfml-audio.so.2.5.1 libsfml-audio.so
    ln -sf libsfml-window.so.2.5.1 libsfml-window.so
    ln -sf libsfml-system.so.2.5.1 libsfml-system.so
    ln -sf libsfml-network.so.2.5.1 libsfml-network.so
    cd ../..
fi
