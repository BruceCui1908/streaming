docker rm -f streaming

docker build -t streaming:debug -f Dockerfile --progress=plain .
docker run -it -d -p 8080:8080 -p 1935:1935 -p 10000:10000/udp --name streaming streaming:debug

docker exec -it streaming /bin/bash