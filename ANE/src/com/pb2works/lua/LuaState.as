package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	import flash.events.*;
	import flash.utils.*;
	import com.pb2works.PB2E;
	import com.pb2works.lua.LuaObject;
	
	public class LuaState extends EventDispatcher {
		private var ctx:ExtensionContext;
		private var extData:Object;
		private var L:int; // Lua state
		private var done:Boolean;

		public function LuaState(host:PB2E) {
			done = false;
			ctx = host.context;
			L = ctx.call("LUA") as int;
			if (L == -1) throw new Error("Lua state limit reached.");
			extData = ctx.actionScriptData;
			extData.ls[L] = this;
			extData.lf[L] = new Dictionary();
			if (extData.mt == undefined) extData.mt = host.getMain();
			if (extData.lo == undefined) extData.lo = LuaObject.make;
		}

		public function addFunction(fname:String, func:Function) : void {
			if (done) return;
			extData.lf[L][fname] = func;
			doString("local AS3 = __AS3; function " + fname + "(...) return AS3(\"" + fname + "\", ...) end" , "internal_redir");
		}

		public function addFunction2(fname, func:Function) : void {
			extData.lf[L][fname] = func;
		}

		public function doString(code:String, where:String) : void {
			if (done) return;
			var out:*;
			out = ctx.call("LUA_DoString", L, code, where);
			if (out != undefined) {
				throw new Error(out as String);
			}
		}

		public function loadAPI(API:LuaAPI) : void {
			for (var fname:String in API.funcs) {
				extData.lf[L][fname] = API.funcs[fname];
			}
			doString(API.api, "internal_loadAPI");
			for each (var obj:String in API.objs) {
				ctx.call("LUA_DefMeta", L, obj);
			}
			doString(API.api_c, "internal_loadAPICleanup");
		}

		public function close() : void {
			extData.ls[L] = null;
			ctx.call("LUA_Close", L);
			done = true;
		}

		public function get closed() : Boolean {
			return done;
		}

		public function register(idx:int) : int {
			return ctx.call("LUA_RegArg", L, idx) as int;
		}

		public function toBool(v:Boolean) : LuaBool {
			return new LuaBool(this, v);
		}

		public function toNumber(v:Number) : LuaNumber {
			return new LuaNumber(this, v, false);
		}

		public function toInteger(v:int) : LuaNumber {
			return new LuaNumber(this, v, true);
		}

		public function callFunction(ridx:int, args:Array) : void {
			var out:* = ctx.call.apply(ctx, ["LUA_CRegFunc", L, ridx].concat(args));
			if (out != null) throw new Error(out);
		}

		public function defineObject(className:String) : void {
			ctx.call("LUA_DefMeta", L, className);
		}

		public function defineProperty(className:String, propertyName:String, getter:Function, setter:Function=null) : void {
			                    extData.lf[L][className + '_get_' + propertyName] = getter;
			if (setter != null) extData.lf[L][className + '_set_' + propertyName] = setter;
		}

		public function defineMethod(className:String, methodName:String, methodFunc:Function) : void {
			extData.lf[L][className + '_' + methodName] = methodFunc;
		}

		// (For now?) this is just supposed to be used as a reference to the internal AS3 IDs
		// Pushes to the stack (i.e. only for function returns)
		public function newObject(className:String, id:int) : LuaObject {
			ctx.call("LUA_NewMeta", L, className, id);
			return new LuaObject(this, LuaObject.TYPE_USERDATA, -1); // Top of the stack
		}

		public function makeString(str:String) : LuaString {
			return new LuaString(this, str);
		}
	}
}