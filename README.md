# Face Comparison Service

## Introduction

A http restful face comparison service based on dlib.

## How to build

```sh
git clone https://github.com/liuzl/facerecognition
cd facerecognition
mkdir build && cd build
cmake .. && cmake --build .
```

## How to make a call

```sh
if [ $# -lt 2 ]; then
    echo "Usage: $0 <img1> <img2>"
    exit 1;
fi

img1=`base64 -i $1 | tr -d '\n' | xxd -plain | sed 's/\(..\)/%\1/g' `
img2=`base64 -i $2 | tr -d '\n' | xxd -plain | sed 's/\(..\)/%\1/g' `

echo "img1=$img1&img2=$img2" > postdata

curl -d @postdata -X POST http://localhost:5000/api/v1/facecompare
rm postdata
```
