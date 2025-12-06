PARSER=parser
DUG=debug
HTTP_SERVER=http_server
cpp=g++

.PHONY:all
all:$(PARSER) $(DUG) $(HTTP_SERVER)

$(PARSER):parser.cpp
	$(cpp) -o $@ $^ -lboost_system -lboost_filesystem -std=c++11
$(DUG):debug.cpp
	$(cpp) -o $@ $^ -std=c++11 -ljsoncpp
$(HTTP_SERVER):http_server.cpp
	$(cpp) -o $@ $^ -std=c++11 -ljsoncpp -lpthread
.PHONY:clean
clean:
	rm -f $(DUG) $(PARSER) $(HTTP_SERVER)