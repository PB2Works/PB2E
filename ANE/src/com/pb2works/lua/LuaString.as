package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	
	// A reference to a Lua object, or a value 
	public class LuaString extends LuaObject {
		public var str:String;

		public function LuaString(state:LuaState, str:String, idx:int=-1) {
			super(state, LuaObject.TYPE_STRING);
			this.str = str;
			this.idx = idx;
		}

		public override function toString() : String {
			return str;
		}
	}
}