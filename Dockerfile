FROM gcc:cpp

ENV mode=prod

EXPOSE 1935/tcp
EXPOSE 80/tcp

COPY . /usr/local/media
WORKDIR /usr/local/media
RUN ls
RUN mv tests/ include/
RUN make
WORKDIR /usr/local/media/build