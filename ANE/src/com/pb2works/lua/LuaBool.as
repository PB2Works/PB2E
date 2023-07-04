package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	
	// A reference to a Lua object, or a value 
	public class LuaBool extends LuaObject {
		public var val:Boolean;

		public function LuaBool(state:LuaState, val:Boolean) {
			super(state, LuaObject.TYPE_BOOL);
			this.val = val;
		}

		public override function toString() : String {
			return val ? "true" : "false";
		}
	}
}