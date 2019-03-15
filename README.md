# Sound2Image
Sound2Image is a music visualizer Real-Time System project.

# Dependencies
- Allegro 5 (https://liballeg.org/index.html)
- FFTW (http://fftw.org)
- libsndfile (http://www.mega-nerd.com/libsndfile)

## macOS
```bash
brew install allegro
brew install fftw --without-fortran
brew install libsndfile 
```

## Ubuntu
```bash
sudo apt install liballegro5-dev
sudo apt install fftw-dev
sudo apt install libsndfile1-dev 
```


# Try it!
Install all dependencies if you did not.

```bash
git clone https://github.com/blackwut/Sound2Image.git
cd ./Sound2Image
make
```