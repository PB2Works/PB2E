package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	import com.pb2works.lua.LuaState;
	import com.pb2works.lua.LuaBool;
	import com.pb2works.lua.LuaNumber;
	import com.pb2works.lua.LuaString;
	import com.pb2works.lua.LuaFunction;
	
	// A reference to a Lua object, or a value 
	public class LuaObject {
		protected var state:LuaState; // For communicating
		public var idx:int;           // Index in the Lua stack (for parameters only)
		public var ridx:int;          // Index in the registry
		public var t:int;
		public static const TYPE_NIL:int       = 0;
		public static const TYPE_BOOL:int      = 1;
		public static const TYPE_LUSERDATA:int = 2;
		public static const TYPE_NUMBER:int    = 3;
		public static const TYPE_STRING:int    = 4;
		public static const TYPE_TABLE:int     = 5;
		public static const TYPE_FUNCTION:int  = 6;
		public static const TYPE_USERDATA:int  = 7;
		public static const TYPE_THREAD:int    = 8;
		public static const TYPE_INT:int       = 13;

		public function LuaObject(state:LuaState, t:int, idx:int=-1) {
			this.state = state;
			this.t = t;
			this.idx = idx;
		}

		public static function typeName(t:int) : String {
			switch (t) {
				case TYPE_NIL:
					return "nil";
				case TYPE_NUMBER:
				case TYPE_INT:
					return "number";
				case TYPE_BOOL:
					return "boolean";
				case TYPE_STRING:
					return "string";
				case TYPE_TABLE:
					return "table";
				case TYPE_FUNCTION:
					return "function";
				case TYPE_THREAD:
					return "coroutine";
				case TYPE_USERDATA:
				case TYPE_LUSERDATA:
					return "userdata";
			}
			return "???"
		}

/*
		public static function make(a:*=undefined, b:*=undefined, v:*=undefined, idx:*=-1) : LuaNumber {
			return new LuaNumber(null, 600, true);
		}
*/

		/*public static function make(a:*=undefined, b:*=undefined, v:*=undefined, idx:*=-1) : LuaObject {
			return new LuaNumber(null, 100, true);
		}*/

		public function toString() : String {
			return "(" + typeName(t) + ")";
		}


		// For internal use by ANE
		public static function make(s:LuaState, t:int, v:*, idx:int=-1) : LuaObject {
			switch (t) {
				case TYPE_BOOL:
					return new LuaBool(s, v);
				case TYPE_INT:
				case TYPE_NUMBER:
					return new LuaNumber(s, v, t == TYPE_INT);
				case TYPE_STRING:
					return new LuaString(s, v, idx);
				case TYPE_FUNCTION:
					return new LuaFunction(s, idx);
			}
			return new LuaObject(s, t);
		}

		public static function assertType(o:LuaObject, t:int) : void {
			if (o.t != t) 
				throw new Error("wrong type, expected '" + typeName(t) + "', but got '" + typeName(o.t) + "'");
		}

		public static function asNumber(o:LuaObject) : Number {
			if (o.t != TYPE_INT && o.t != TYPE_NUMBER)
				throw new Error("wrong type, expected '" + typeName(TYPE_NUMBER) + "', but got '" + typeName(o.t) + "'");
			return Number((o as LuaNumber).val);
		}

		public static function asString(o:LuaObject) : String {
			assertType(o, TYPE_STRING);
			return (o as LuaString).str;
		}

		public static function asBool(o:LuaObject) : Boolean {
			assertType(o, TYPE_BOOL);
			return (o as LuaBool).val;
		}

		public static function asLuaFunction(o:LuaObject) : LuaFunction {
			assertType(o, TYPE_FUNCTION);
			return o as LuaFunction;
		}
	}
}