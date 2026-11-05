package cases.client;

import haxe.Constraints.IMap;

// Script implementations of a host interface whose functions use Dynamic for type parameters

private class StringStringMap implements IMap<String, String> {
	final map = new Map<String, String>();

	public function new() {}

	public function get(k:String):Null<String> return map.get(k);
	public function set(k:String, v:String):Void map.set(k, v);
	public function exists(k:String):Bool return map.exists(k);
	public function remove(k:String):Bool return map.remove(k);
	public function keys():Iterator<String> return map.keys();
	public function iterator():Iterator<String> return map.iterator();
	public function keyValueIterator():KeyValueIterator<String, String> return map.keyValueIterator();
	public function copy():IMap<String, String> return null;
	public function toString():String return map.toString();
	public function clear():Void map.clear();
	public function size():Int return map.size();
}

private class IntFloatMap implements IMap<Int, Float> {
	final map = new Map<Int, Float>();

	public function new() {}

	public function get(k:Int):Null<Float> return map.get(k);
	public function set(k:Int, v:Float):Void map.set(k, v);
	public function exists(k:Int):Bool return map.exists(k);
	public function remove(k:Int):Bool return map.remove(k);
	public function keys():Iterator<Int> return map.keys();
	public function iterator():Iterator<Float> return map.iterator();
	public function keyValueIterator():KeyValueIterator<Int, Float> return map.keyValueIterator();
	public function copy():IMap<Int, Float> return null;
	public function toString():String return map.toString();
	public function clear():Void map.clear();
	public function size():Int return map.size();
}

class NativeInterfaceTypes extends Test {
	function testStringStringMap() {
		final m:IMap<String, String> = new StringStringMap();
		m.set("a", "x");
		Assert.equals("x", m.get("a"));
		Assert.isTrue(m.exists("a"));
		Assert.isFalse(m.exists("b"));
		Assert.equals(1, m.size());
		Assert.isTrue(m.remove("a"));
		Assert.isNull(m.get("a"));
	}

	function testIntFloatMap() {
		final m:IMap<Int, Float> = new IntFloatMap();
		m.set(1, 1.5);
		Assert.equals(1.5, m.get(1));
		Assert.isTrue(m.exists(1));
		Assert.isFalse(m.exists(2));
		Assert.isTrue(m.remove(1));
		Assert.isFalse(m.exists(1));
	}
}
