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
Install all dependencies if you did not. Then:

```bash
git clone https://github.com/blackwut/Sound2Image.git
cd ./Sound2Image
make
```

Run the program:

``` bash
./Sound2Image wav/ok.wav
```

## User interaction

| Key         | Action                |
| ----------- | --------------------- |
| Arrow Up    | Bigger Bubbles        |
| Arrow Down  | Smaller Bubbles       |
| Arrow Left  | Less Bubbles          |
| Arrow Right | More Bubbles          |
| +           | Higher Volume         |
| -           | Lower Volume          |
| 1           | Rectangular Windowing |
| 2           | Welch Windowing       |
| 3           | Triangular Windowing  |
| 4           | Barlett Windowing     |
| 5           | Hanning Windowing     |
| 6           | Hamming Windowing     |
| 7           | Blacman Windowing     |
