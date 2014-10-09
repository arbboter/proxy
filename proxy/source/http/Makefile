#SDK MAKEFILE 1.0
###########################################################################
DIR:=../..
OBJECT:=$(DIR)/lib/libhttpclient.a
SOURCES:=$(wildcard *.cpp)

DELETE:=$(patsubst %.cpp,%.o,$(SOURCES))
DELETE+=$(patsubst %.cpp,%.d,$(SOURCES))
###########################################################################
-include $(DIR)/main/Comdef
-include $(DIR)/main/build
###########################################################################
