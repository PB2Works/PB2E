ACOMPC = ${AIR_SDK_HOME}/bin/acompc
ADT    = ${AIR_SDK_HOME}/bin/adt

ANE  = PB2E.ane
SWC  = pb2e.swc
LIB  = PB2E.dll
DESC = descriptor.xml

EID        = com.pb2works.PB2E
CLASSES    = com.pb2works.PB2E com.pb2works.lua.LuaState com.pb2works.lua.LuaObject com.pb2works.lua.LuaBool com.pb2works.lua.LuaNumber com.pb2works.lua.LuaString com.pb2works.lua.LuaFunction com.pb2works.lua.LuaAPI com.pb2works.net.NativeDatagramSocket
PLATFORM   = Windows-x86
VERSION    = 14

PROJ = "$(PB2EX_DIR)\\pb2_re34_alt_p.app"
EXTF = \\META-INF\\AIR\\extensions\\$(EID)\\META-INF\\ANE\\$(PLATFORM)\\

$(ANE): $(SWC) $(DESC) # $(LIB)
	./7z -y e -x!catalog.xml $(SWC)
	move /Y library.swf platforms/$(PLATFORM)
	$(ADT) -package -target ane $(ANE) $(DESC) -swc $(SWC) -platform $(PLATFORM) -C platforms/$(PLATFORM) .
	copy platforms\\Windows-x86\\library.swf $(PROJ)$(EXTF)
	copy $(ANE) $(PROJ)\\..\\

$(SWC): src/com/pb2works/*.as src/com/pb2works/lua/*.as src/com/pb2works/net/*.as
	$(ACOMPC) -source-path src -include-classes $(CLASSES) -swf-version=$(VERSION) -output $(SWC)