package com.pb2works
{
	import flash.external.ExtensionContext;
	import flash.events.*;
	import flash.display.Stage;
	import flash.display.MovieClip;

	public class PB2E extends EventDispatcher {
		private var ctx:ExtensionContext;
		private var extData:Object;
		private var stage:Stage;
		private var main:MovieClip;

		public function PB2E(main:MovieClip) {
			this.stage = main.stage;
			this.main = main;
			ctx = ExtensionContext.createExtensionContext("com.pb2works.PB2E", null);
			ctx.addEventListener(StatusEvent.STATUS, function(ev:StatusEvent) : void {
				if (ev.code == "key") {
					var kev:KeyboardEvent;
					pollKeys();
					// kev = new KeyboardEvent(ev.level == "up" ? KeyboardEvent.KEY_UP : KeyboardEvent.KEY_DOWN, true, false, extData.k_char, extData.k_code);
					// dispatchEvent(new KeyboardEvent(KeyboardEvent.KEY_DOWN));
					// dispatchEvent(kev);
					for (var i:int = 0; i < extData.k_n; i++) {
						dispatchEvent(extData.k_ev[i]);
					}
				} else if (ev.code == "ms") {
					if (ev.level == "d") dispatchEvent(new MouseEvent(MouseEvent.MOUSE_DOWN));
					else dispatchEvent(new MouseEvent(MouseEvent.MOUSE_UP));
				}
			});
			extData = ctx.actionScriptData;

			ctx.call("setStageSize", stage.stageWidth, stage.stageHeight);
		}

		public function getMain() : MovieClip {
			return main;
		}

		public function get context() : ExtensionContext {
			return ctx;
		}

		private function pollKeys() : void {
			ctx.call("pollKeys");
		}

		public function pollMouse() : void {
			ctx.call("pollMouse");
		}

		public function get mouseX() : Number {
			return Number(extData.mx);
		}

		public function get mouseY() : Number {
			return Number(extData.my);
		}

		public function hmm() : * {
			return extData.k_n + " " + extData.k_ev;
		}

		public function helloWorld() : String  {
			// Calls the native function helloWorld. Name must match the
			// name specified to the runtime during the extensionContext's
			// initialization, otherwise you will obviously get error #3500.
			return ctx.call("helloWorld") as String;
		}

		public function helloProsu() : int {
			return ctx.call("helloProsu") as int;
		}
	}
}