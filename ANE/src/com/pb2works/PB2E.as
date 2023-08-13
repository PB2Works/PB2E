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
		private var measures:Vector.<uint>;
		private var perfFactor:Number;

		public function PB2E(main:MovieClip) {
			this.stage = main.stage;
			this.main = main;
			trace("[PB2E] Creating extension");
			ctx = ExtensionContext.createExtensionContext("com.pb2works.PB2E", null);
			ctx.addEventListener(StatusEvent.STATUS, function(ev:StatusEvent) : void {
				if (ev.code == "key") {
					pollKeys();
					for (var i:int = 0; i < extData.k_n; i++) dispatchEvent(extData.k_ev[i]);
				} else if (ev.code == "ms") {
					if (ev.level == "d") dispatchEvent(new MouseEvent(MouseEvent.MOUSE_DOWN));
					else dispatchEvent(new MouseEvent(MouseEvent.MOUSE_UP));
				}
			});
			extData = ctx.actionScriptData;

			ctx.call("setStageSize", stage.stageWidth, stage.stageHeight);

			measures = new Vector.<uint>(20);
			perfFactor = 1000000.0 / Number(ctx.call("perfFrequency") as uint);
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

		public function getTime(n:uint) : Number {
			var v:Number = measures[n] * perfFactor;
			measures[n] = 0.0; 
			return v;
		}

		public function startMeasure(n:uint) : void {
			ctx.call("t1", n);
		}

		public function stopMeasure(n:uint) : void {
			measures[n] += ctx.call("t2", n) as uint;
		}
	}
}