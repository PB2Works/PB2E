package com.pb2works.lua
{
	import flash.external.ExtensionContext;
	import flash.events.*;
	import flash.utils.*;
	import com.pb2works.PB2E;
	import com.pb2works.lua.LuaObject;
	
	public class LuaAPI {
		public  var api:String;
		public  var api_c:String;
		public  var funcs:Dictionary;
		public  var gfuncs:Dictionary;
		public  var objs:Vector.<String>;
		private var setters:Dictionary;
		private var getters:Dictionary;
		private var methods:Dictionary;


		public function LuaAPI() {
			api     = "";
			api_c   = "";
			funcs   = new Dictionary();
			gfuncs  = new Dictionary();
			objs    = new Vector.<String>();
			setters = new Dictionary();
			getters = new Dictionary();
			methods = new Dictionary();
		}

		// Turn objs, setters, getters and methods into API Lua code
		public function make() : void {
			api = "local AS3 = __AS3\n";
			api_c = "__AS3 = nil\n";

			// ----------------
			// Global functions
			// ----------------
			for (var fname:String in gfuncs) {
				api += "function " + fname + "(...) return AS3(\"" + fname + "\", ...) end\n";
			}

			// -------
			// Objects
			// -------
			for each (var obj:String in objs) {
				var pname:String;

				api += obj + ' = {}\n';
				api_c += obj + ' = nil\n';

				// ==========
				//    AS3
				// ==========

				// AS3 Setters
				api += 'local st = {\n';
				for each (pname in setters[obj]) {
					api += '	' + pname + ' = "0' + obj + '_set_' + pname + '",\n';
				}
				api += '}\n';

				// AS3 Getters
				api += 'local gt = {\n';
				for each (pname in getters[obj]) {
					api += '	' + pname + ' = "0' + obj + '_get_' + pname + '",\n';
				}
				api += '}\n';

				// AS3 Methods
				api += 'local mt = {\n';
				for each (pname in methods[obj]) {
					api += '	' + pname + ' = function(...) return AS3("0' + obj + '_' + pname + '", ...) end,\n';
				}
				api += '}\n';


				// ===========
				// Metamethods
				// ===========

				// __index
				api += 'function ' + obj + ':__index(x)\n';
				api += '	local f = gt[x]\n';
				api += '	if f ~= nil then return AS3(f, self) end\n';
				api += '	f = mt[x]\n';
				api += '	if f ~= nil then return f end\n';
				api += '	return nil\n'
				api += 'end\n'

				// __newindex
				api += 'function ' + obj + ':__newindex(x, v)\n';
				api += '	local f = st[x]\n';
				api += '	if f ~= nil then AS3(f, self, v) end\n';
				api += '	return nil\n';
				api += 'end\n';
			}
		}

		public function addFunction(name:String, func:Function) : void {
			gfuncs[name] = func;
			funcs[name] = func;
		}

		public function defineObject(className:String) : void {
			objs.push(className);
			setters[className] = new Vector.<String>();
			getters[className] = new Vector.<String>();
			methods[className] = new Vector.<String>();
		}

		public function defineProperty(className:String, propertyName:String, getter:Function, setter:Function=null) : void {
			getters[className].push(propertyName);
			funcs[className + "_get_" + propertyName] = getter;
			if (setter != null) {
				setters[className].push(propertyName);
				funcs[className + "_set_" + propertyName] = setter;
			}
		}

		public function defineMethod(className:String, methodName:String, methodFunc:Function) : void {
			methods[className].push(methodName);
			funcs[className + '_' + methodName] = methodFunc;
		}
	}
}