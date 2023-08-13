package com.pb2works.net 
{
	import com.pb2works.PB2E;
	import flash.external.ExtensionContext;
	import flash.events.EventDispatcher;
	import flash.events.DatagramSocketDataEvent;
	import flash.utils.ByteArray;

	public class NativeDatagramSocket extends EventDispatcher {
		public static const isSuported:Boolean = true;

		private var ctx:ExtensionContext;
		private var _bound:Boolean     = false;
		private var _connected:Boolean = false;
		private var receiving:Boolean  = false;
		private var closed:Boolean     = false;
		private var id:int;

		public function NativeDatagramSocket(host:PB2E) {
			ctx = host.context;
			id = ctx.call("NET_NewUDP") as int;
		}

		private function lookupDomain(domain:String) : String {
			return ctx.call("NET_DNS", domain) as String;
		}

		public function get bound() : Boolean {
			return _bound;
		}

		public function get connected() : Boolean {
			return _connected;
		}

		public function get localAddress() : String {
			return "";
		}

		public function get localPort() : int {
			return -1;
		}

		public function get remoteAddress() : String {
			return "";
		}

		public function get remotePort() : int {
			return -1;
		}

		public function bind(localPort:int = 0, localAddress:String = "0.0.0.0") : void {
			_bound = true;
			throw new Error("NativeDatagramSocket.bind is not supported.");
		}

		public function connect(remoteAddress:String, remotePort:int) : void {
			_connected = true;
			throw new Error("NativeDatagramSocket.connect is not supported.");
			//ctx.call("Net_ConnectUDP", id, remoteAddress, remotePort);
		}

		public function close() : void {
			if (closed) return;
			closed = true;
			receiving = false;
			ctx.call("NET_CloseUDP", id);
			id = -1;
		}

		public function receive() : void {
			receiving = true;
		}

		public function send(bytes:ByteArray, offset:uint = 0, length:uint = 0, address:String = null, port:int = 0) : void {
			if (closed) return;
			var actualLength:int = bytes.length;
			address = lookupDomain(address);
			if (address == null) {
				throw new Error("NativeDatagramSocket.send: invalid address");
			}
			if (port < 1 || port > 65535) throw new Error("NativeDatagramSocket.send: wrong port (" + port + ")");
			if (offset < 0 || offset >= actualLength || offset + length > actualLength) 
				throw new Error("NativeDatagramSocket.send: out of bounds");
			ctx.call("NET_SendUDP", id, bytes, offset, actualLength - offset, address, port);
		}

		private function fireDataEvent(data:ByteArray) : void {
			dispatchEvent(new DatagramSocketDataEvent(DatagramSocketDataEvent.DATA, false, false, "", 0, "", 0, data));
		}

		public function poll() : void {
			if (!receiving) return;
			ctx.call("NET_PollUDP", id, this, fireDataEvent);
		}
	}
}