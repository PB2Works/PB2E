package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	
	// A reference to a Lua object, or a value 
	public class LuaNumber extends LuaObject {
		public var val:*;
		public var isint:Boolean;

		public function LuaNumber(state:LuaState, v:*, isint:Boolean) {
			super(state, isint ? LuaObject.TYPE_INT : LuaObject.TYPE_NUMBER);
			val = v;
			this.isint = isint;
		}

		public override function toString() : String {
			return String(isint ? (val as int) : (val as Number));
		}
	}
}