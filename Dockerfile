FROM gcc:cpp

ENV mode=prod

COPY . /usr/local/media
WORKDIR /usr/local/media
RUN ls
RUN mv tests/ include/
RUN make
WORKDIR /usr/local/media/build