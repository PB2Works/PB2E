#include <windows.h>
#include "lua.hpp"
#include "as3lua.h"

// lua types: 
//   Primitive (passed by value):   nil,   boolean,  number,   string
//   Complex (passed by reference): table, function, userdata, thread
// number is interesting because it can be either a double or an int internally (i.e. 2 types that show as 1)
// question: Should C convert AS3 primitives (undefined, null, Number, int/uint, Boolean) to Lua primities (nil, boolean, number, string)?
//           And what about the other way (Lua primitive -> AS3 primitive) in LUA API arguments?

#define LAS3_REG_NAME "pb2wr"
#define LAS3_TYPE_INT 13
#define LAS3_MAX_STATES 10

struct luaextra {
	int Ln;            // The index of the state in the luaStates array
	FREContext ctx;    // The extension context
	FREObject asData;  // actionScriptData - (store AS3 objects here that you don't want GC'ed [or not])
	FREObject state;   // The AS3 LuaState instance
	FREObject api;     // Lua API functions
	FREObject lo;      // Lua object constructor
	FREObject mt;      // Reference to MainTimeline
};

// Userdata struct
struct luamobj {
	int id; // Internal ID of object (player, region, moveable, etc.)
};

static lua_State* luaStates[LAS3_MAX_STATES];

// Lua native value -> AS3 native type
inline FREObject getRawLuaValue(lua_State* L, int idx) {
	FREObject arg = NULL;
	FREObject asException;
	switch (lua_type(L, idx)) {
	case LUA_TBOOLEAN:
		FRENewObjectFromBool(lua_toboolean(L, idx), &arg);
		break;
	case LUA_TNUMBER:
		if (lua_isinteger(L, idx)) FRENewObjectFromInt32(lua_tointeger(L, idx), &arg);
		else FRENewObjectFromDouble(lua_tonumber(L, idx), &arg);
		break;
	case LUA_TSTRING: {
		const char* str = lua_tostring(L, idx);
		FRENewObjectFromUTF8(strlen(str), _AIRS(str), &arg);
		break;
	}
	default:
		FRENewObject(_AIRS("Object"), 0, NULL, &arg, &asException);
		break;
	}
	return arg;
}

// Lua native value -> AS3 LuaObject
inline FREObject getLuaValue(lua_State* L, FREObject state, FREObject lo, int idx) {
	/* LuaObject.make args
	* argv[0] - this
	* argv[1] - state (LuaState)
	* argv[2] - type (int)
	* argv[3] - value (internal AS3 representation of Lua value)
	* argv[4] - index in the stack (only for non-primitive types so they could be referenced)
	*/
	FREObject obj, argv[5], asException;
	argv[0] = NULL;
	argv[1] = state;
	int type = lua_type(L, idx);
	switch (type) {
	case LUA_TBOOLEAN:
		FRENewObjectFromInt32(type, &argv[2]);
		FRENewObjectFromBool(lua_toboolean(L, idx), &argv[3]);
		FRECallObjectMethod(lo, _AIRS("call"), 4, argv, &obj, &asException);
		break;
	case LUA_TNUMBER:
		if (lua_isinteger(L, idx)) {
			FRENewObjectFromInt32(LAS3_TYPE_INT, &argv[2]);
			FRENewObjectFromInt32(lua_tointeger(L, idx), &argv[3]);
			// printf("[LUA] ---- Passing integer at index %d with value %d\n", idx, lua_tointeger(L, idx));
		}
		else {
			FRENewObjectFromInt32(type, &argv[2]);
			FRENewObjectFromDouble(lua_tonumber(L, idx), &argv[3]);
		}
		if (FRECallObjectMethod(lo, _AIRS("call"), 4, argv, &obj, &asException) != FRE_OK) {
			FREObject asException2, asExceptionMessage;
			uint32_t asExceptionMessageLength;
			const uint8_t* asExceptionString;
			FREGetObjectProperty(asException, _AIRS("message"), &asExceptionMessage, &asException2);
			FREGetObjectAsUTF8(asExceptionMessage, &asExceptionMessageLength, &asExceptionString);
			printf("[LUA] LuaObject.make error: %s\n", asExceptionString);
		}
		break;
	case LUA_TSTRING: {
		FRENewObjectFromInt32(type, &argv[2]);
		const char* str = lua_tostring(L, idx);
		FRENewObjectFromUTF8(strlen(str), _AIRS(str), &argv[3]);
		FRECallObjectMethod(lo, _AIRS("call"), 4, argv, &obj, &asException);
		break;
	}
	case LUA_TNIL:
		FRENewObjectFromInt32(type, &argv[2]);
		argv[3] = NULL;
		FRECallObjectMethod(lo, _AIRS("call"), 4, argv, &obj, &asException);
		break;
	default:
		FRENewObjectFromInt32(type, &argv[2]);
		argv[3] = NULL;
		FRENewObjectFromInt32(idx, &argv[4]);
		FRECallObjectMethod(lo, _AIRS("call"), 5, argv, &obj, &asException);
		break;
	}
	return obj;
}

// AS3 LuaObject -> Lua native value
// Returns: # of argumenets (preferrably 0 or 1)
inline int pushAS3Value(lua_State* L, FREObject value) {
	FREObject asException, prop, propt;
	FREObjectType asType;
	int valueType;
	int result;
	if (value == 0) {
		valueType = LUA_TNIL;
	} else {
		FREGetObjectType(value, &asType);
		if (asType != FRE_TYPE_OBJECT) valueType = LUA_TNIL;
		else {
			result = FREGetObjectProperty(value, _AIRS("t"), &propt, &asException);
			FREGetObjectAsInt32(propt, &valueType);
		}
	}
	switch (valueType) {
	case LUA_TBOOLEAN: {
		uint32_t vBool;
		FREGetObjectProperty(value, _AIRS("val"), &prop, &asException);
		FREGetObjectAsBool(prop, &vBool);
		lua_pushboolean(L, vBool);
		break;
	}
	case LAS3_TYPE_INT: {
		int vInt;
		FREGetObjectProperty(value, _AIRS("val"), &prop, &asException);
		FREGetObjectAsInt32(prop, &vInt);
		lua_pushinteger(L, vInt);
		break;
	}
	case LUA_TNUMBER: {
		double vDouble;
		FREGetObjectProperty(value, _AIRS("val"), &prop, &asException);
		FREGetObjectAsDouble(prop, &vDouble);
		lua_pushnumber(L, vDouble);
		break;
	}
	case LUA_TSTRING: {
		const uint8_t* vStr;
		uint32_t vLen;
		FREGetObjectProperty(value, _AIRS("str"), &prop, &asException);
		FREGetObjectAsUTF8(prop, &vLen, &vStr);
		lua_pushstring(L, (const char*)vStr);
		break;
	}
					// Just make sure to create userdata in the right order
	case LUA_TUSERDATA: { // Assuming the userdata is on the stack (which it should be)
		/*FREObject asIdx;
		int idx;
		FREGetObjectProperty(value, _AIRS("idx"), &asIdx, &asException);
		FREGetObjectAsInt32(asIdx, &idx);
		lua_insert(L, idx);*/
		printf("RETURNING USERDATA.... %d\n", lua_gettop(L));
		break;
	}
	default:
		lua_pushnil(L);
		break;
	}
	return 1; // Always have one return value
}

// Get all of the AS3 refs we need
inline void lua_prep(lua_State* L, FREContext ctx) {
	FREObject asData, lfs, asException, ls;
	luaextra* Le = *((luaextra**)lua_getextraspace(L));
	FREGetContextActionScriptData(ctx, &asData);
	FREGetObjectProperty(asData, _AIRS("lf"), &lfs, &asException);
	FREGetArrayElementAt(lfs, Le->Ln, &(Le->api)); // Le->api = actionScriptData.lf[Ln];
	FREGetObjectProperty(asData, _AIRS("lo"), &(Le->lo), &asException); // Le->lo = actionScriptData.lo;
	FREGetObjectProperty(asData, _AIRS("ls"), &ls, &asException);
	FREGetArrayElementAt(ls, Le->Ln, &Le->state);
}

/*
* argv[0]  - Lua state   (int)
* argv[1]  - Registry ID (int)
* argv[2+] - args        (LuaObject)
*/
ANEFunction(LUA_CallRegisteredFunction) {
	int Ln;
	lua_State* L;
	int rid;
	if (argc >= 2) {
		FREGetObjectAsInt32(argv[0], &Ln);
		FREGetObjectAsInt32(argv[1], &rid);
		L = luaStates[Ln];
		lua_prep(L, ctx);
		lua_getfield(L, LUA_REGISTRYINDEX, LAS3_REG_NAME);
		lua_rawgeti(L, -1, rid);
		for (uint32_t i = 2; i < argc; i++) pushAS3Value(L, argv[i]);
		// printf("[LUA %d] CallRegisteredFunction(%d, %d) with %d args\n", Ln, Ln, rid, argc-2);
		if (lua_pcall(L, argc - 2, 0, 0) != LUA_OK) {
			FREObject asErr;
			const char* errs = lua_tostring(L, -1);
			printf("/// The actual error [%d]: %s\n", lua_type(L, -1), errs);
			lua_pop(L, 1);
			FRENewObjectFromUTF8(strlen(errs), _AIRS(errs), &asErr);
			return asErr;
		}
		else {

		}
	}
	return NULL;
}

// AS3 native value -> Lua native value
inline int pushRawAS3Value(lua_State* L, FREObject value) {
	int ret = 1;
	FREObjectType valueType;
	FREGetObjectType(value, &valueType);
	switch (valueType) {
	case FRE_TYPE_BOOLEAN: {
		uint32_t vBool;
		FREGetObjectAsBool(value, &vBool);
		lua_pushboolean(L, vBool);
		break;
	}
	case FRE_TYPE_NUMBER: {
		double vDouble;
		FREGetObjectAsDouble(value, &vDouble);
		lua_pushnumber(L, vDouble);
		break;
	}
	case FRE_TYPE_STRING: {
		const uint8_t* vStr;
		uint32_t vLen;
		FREGetObjectAsUTF8(value, &vLen, &vStr);
		lua_pushstring(L, (const char*)vStr);
		break;
	}
	default:
		ret = 0;
		break;
	}
	return ret;
}

// API function responder (__AS3)
static int lua_AS3(lua_State* L) {
	FREObject as3f, asException, *args, out;
	const char* lfn;
	char* func;
	int rets;
	uint8_t ismeta;
	luamobj* mobj;
	int argc = lua_gettop(L);
	if (argc >= 1) {
		luaextra* Le = *((luaextra**)lua_getextraspace(L));
		lfn = lua_tostring(L, 1);
		ismeta = lfn[0] == '0';
		func = (char*)&lfn[ismeta];
		// printf("[LUA %d] Calling AS3 \"%s\" with %d args.\n", Le->Ln, func, argc - 1);
		FREGetObjectProperty(Le->api, _AIRS(func), &as3f, &asException);

		argc++;
		args = (FREObject*)malloc(sizeof(FREObject) * argc);
		args[0] = NULL; // For AS3's Function.call, first argument is "this", so we put null
		args[1] = Le->state;
		if (ismeta) {
			mobj = (luamobj*)lua_touserdata(L, 2);
			FRENewObjectFromInt32(mobj->id, &args[2]);
		}
		for (int i = 2 + ismeta; i < argc; i++)
			args[i] = getLuaValue(L, Le->state, Le->lo, i);

		if (FRECallObjectMethod(as3f, _AIRS("call"), argc, args, &out, &asException) != FRE_OK) {
			FREObject ex2, eMsg;
			uint32_t msgl;
			const uint8_t* eMsgs;
			FREGetObjectProperty(asException, _AIRS("message"), &eMsg, &ex2);
			FREGetObjectAsUTF8(eMsg, &msgl, &eMsgs);
			printf("[LUA %d] AS3 error: %s\n", Le->Ln, eMsgs);
			rets = luaL_error(L, "[%s]", eMsgs);
			free(args);
			return rets;
		}
		rets = pushAS3Value(L, out);
		free(args);
		return rets;
		// return pushRawAS3Value(L, out);
	}
	return 0;
}

/* Execute Lua code in string form
*  argv[0] - Lua state : int
*  argv[1] - Code      : String
*  argv[2] - Where     : String
*  return  - Error     : String?
*/
ANEFunction(LUA_DoString) {
	lua_State* L;
	int Ln;
	const char* code, * where;
	uint32_t codeLen, whereLen;
	FREObject ret = NULL;
	if (argc >= 3) {
		FREGetObjectAsInt32(argv[0], &Ln);
		FREGetObjectAsUTF8(argv[1], &codeLen, (const uint8_t**)&code);
		FREGetObjectAsUTF8(argv[2], &whereLen, (const uint8_t**)&where);

		L = luaStates[Ln];
		lua_prep(L, ctx);
		if (luaL_loadbuffer(L, code, codeLen, where) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
			const char* err = lua_tostring(L, -1);
			printf("[LUA %d] Error: %s\n", Ln, err);
			FRENewObjectFromUTF8(strlen(err), _AIRS(err), &ret);
			lua_pop(L, 1);
		}
	}
	return ret;
}

ANEFunction(LUA_RegisterArgument) {
	if (argc >= 2) {
		FREObject asRid;
		int32_t Ln, idx;
		lua_State* L;
		int rid;
		FREGetObjectAsInt32(argv[0], &Ln);
		FREGetObjectAsInt32(argv[1], &idx);

		L = luaStates[Ln];
		lua_getfield(L, LUA_REGISTRYINDEX, LAS3_REG_NAME);
		lua_pushvalue(L, idx);
		rid = luaL_ref(L, -2);
		lua_pop(L, 1);
		FRENewObjectFromInt32(rid, &asRid);
		return asRid;
	}
	return NULL;
}

/* Add global table Classname as metatable in registry
*  argv[0] - Lua state : int
*  argv[1] - Classname : String
*/
ANEFunction(LUA_RegisterGlobalMetatable) {
	if (argc >= 2) {
		lua_State* L;
		int Ln;
		uint32_t classNameLen;
		const uint8_t* className;
		FREGetObjectAsInt32(argv[0], &Ln);
		FREGetObjectAsUTF8(argv[1], &classNameLen, &className);

		L = luaStates[Ln];
		printf("%d Registering global metatable: \"%s\"\n", lua_gettop(L), className);
		luaL_newmetatable(L, (const char*)className);
		lua_getglobal(L, (const char*)className);
		lua_pushstring(L, "__index");
		lua_rawget(L, -2);
		lua_pushstring(L, "__index");
		lua_pushvalue(L, -2);
		lua_rawset(L, -5);
		printf("Done registering: %d\n", lua_gettop(L));
		lua_pop(L, 1);
		lua_pushstring(L, "__newindex");
		lua_rawget(L, -2);
		lua_pushstring(L, "__newindex");
		lua_pushvalue(L, -2);
		lua_rawset(L, -5);
		lua_pop(L, 3);
	}
	return NULL;
}

// TODO: Think of a system of storing arbitrary data in userdata. For now, only use ID
// TODO: Freeing userdata not in use
/* Instantiate metatable Classname, and give it internal ID id & push it to the stack for return
*  argv[0] - Lua state : int
*  argv[1] - Classname : String
*  argv[2] - id        : int
*/
ANEFunction(LUA_NewMetaObject) {
	FREObject asId;
	if (argc >= 3) {
		lua_State* L;
		int Ln;
		uint32_t classNameLen;
		const uint8_t* className;
		int id;
		luamobj* mobj;
		FREGetObjectAsInt32(argv[0], &Ln);
		FREGetObjectAsUTF8(argv[1], &classNameLen, &className);
		FREGetObjectAsInt32(argv[2], &id);

		L = luaStates[Ln];
		mobj = (luamobj*)lua_newuserdata(L, sizeof(luamobj));
		mobj->id = id;
		printf("+++++++++ NEW META OBJECT: %ld & %d\n", (int) mobj, lua_gettop(L));
		luaL_setmetatable(L, (const char*)className);
		FRENewObjectFromInt32(1, &asId);
		// lua_pop(L, 1);
	}
	else {
		FRENewObjectFromInt32(0, &asId);
	}
	return asId;
}

ANEFunction(LUA) {
	FREObject as3int;
	lua_State* L;
	luaextra* Le;
	for (int Ln = 0; Ln < LAS3_MAX_STATES; Ln++) {
		if (luaStates[Ln] == NULL) {
			Le = (luaextra*)malloc(sizeof(luaextra));
			Le->Ln = Ln;
			Le->ctx = ctx;
			printf("[LUA] Made state with ID %d\n", Ln);
			FRENewObjectFromInt32(Ln, &as3int);
			L = luaL_newstate();
			luaStates[Ln] = L;
			luaL_openlibs(L);
			*((luaextra**)lua_getextraspace(L)) = Le;
			lua_register(L, "__AS3", lua_AS3);
			lua_createtable(L, 10, 10); // There's probably a more optimal init config
			lua_setfield(L, LUA_REGISTRYINDEX, LAS3_REG_NAME);
			return as3int;
		}
	}
	FRENewObjectFromInt32(-1, &as3int);
	return as3int;
}

ANEFunction(LUA_Close) {
	int Ln;
	lua_State* L;
	luaextra* Le;
	if (argc >= 1) {
		FREGetObjectAsInt32(argv[0], &Ln);
		L = luaStates[Ln];
		Le = *((luaextra**)lua_getextraspace(L));
		if (Ln >= 0 && Ln < LAS3_MAX_STATES && (L != NULL)) {
			printf("[LUA %d] Closing... %d == %d\n", Le->Ln, (int)Le->ctx, (int)ctx);
			lua_close(L);
			free(Le);
			luaStates[Ln] = NULL;
		}
	}
	return NULL;
}

ANEFunction(LUA_GetError) {
	return NULL;
}

ANEFunction(LUA_SetGlobal) {
	return NULL;
}

ANEFunction(LUA_GetGlobal) {
	if (argc >= 2) {
		lua_State* L;
		int Ln;
		uint32_t nameLen;
		const uint8_t* name;
		FREGetObjectAsInt32(argv[0], &Ln);
		FREGetObjectAsUTF8(argv[1], &nameLen, &name);

		L = luaStates[Ln];
		lua_getglobal(L, (const char*)name);

		lua_pop(L, 1);
	}
	return NULL;
}

ANEFunction(LUA_CallFunction) {
	return NULL;
}

void lua_init(FREContext ctx) {
	FREObject actionScriptData, vect, asException, asInt;
	for (int i = 0; i < LAS3_MAX_STATES; i++) luaStates[i] = NULL;

	FREGetContextActionScriptData(ctx, &actionScriptData);
	FRENewObjectFromUint32(LAS3_MAX_STATES, &asInt);
	FRENewObject(_AIRS("Vector.<flash.utils.Dictionary>"), 1, &asInt, &vect, &asException);
	FRESetObjectProperty(actionScriptData, _AIRS("lf"), vect, &asException);
	FRENewObject(_AIRS("Vector.<com.pb2works.lua.LuaState>"), 1, &asInt, &vect, &asException);
	FRESetObjectProperty(actionScriptData, _AIRS("ls"), vect, &asException);
}

void lua_close() {
}