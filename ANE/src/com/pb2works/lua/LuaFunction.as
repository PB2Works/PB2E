package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	
	// A reference to a Lua object, or a value 
	public class LuaFunction extends LuaObject {
		public function LuaFunction(state:LuaState, idx:int) {
			super(state, LuaObject.TYPE_FUNCTION);
			this.idx = idx;
		}

		public function getState() : LuaState {
			return state;
		}

		public function register() : int {
			ridx = state.register(idx);
			return ridx;
		}

		public function run(...args) : void {
			state.callFunction(ridx, args);
		}
	}
}