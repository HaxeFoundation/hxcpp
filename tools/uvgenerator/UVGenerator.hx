import haxe.Exception;
import sys.io.File;
import eval.luv.File.FileSync;
import haxe.io.Path;
import haxe.PosInfos;
import eval.luv.Dir.DirSync;

using StringTools;

typedef CType = {
	name:String,
	stars:Int,
	const:Bool,
	unsigned:Bool,
	struct:Bool
}

typedef TypeAndName = {
	type:CType,
	name:String,
	array:Int,
}

typedef FunctionSignature = {
	var name:String;
	var returnType:CType;
	var arguments:Array<TypeAndName>;
}

typedef StructSignature = {
	var type:String;
	var name:String;
	var fields:Array<TypeAndName>;
}

class Skip extends Exception {}

class UVGenerator {
	static inline var DOCS = 'project/thirdparty/libuv-1.42.0/docs/src';
	static inline var UV_HX = 'tools/uvgenerator/UV.hx';
	static inline var UV_HX_HEADER = 'tools/uvgenerator/UV.hx.header';

	static final skipDocs = ['api', 'dll', 'guide', 'index', 'migration_010_100',
		'poll', 'threading', 'threadpool', 'upgrading'];

	static final predefinedHxTypes = new Map<String,String>();
	static final hxTypesToGenerate = new Map<String,String>();

	static function main() {
		var root = rootDir();
		trace(root);
		var docsDir = Path.join([root, DOCS]);
		var scan = DirSync.scan(docsDir).resolve();
		var hxFile = File.write(Path.join([root, UV_HX]));

		var hxHeader = File.getContent(Path.join([root, UV_HX_HEADER]));
		collectPredefinedHxTypesToMap(hxHeader, predefinedHxTypes);
		hxFile.writeString(hxHeader);

		var entry = null;
		while(null != (entry = scan.next())) {
			if(entry.kind != FILE || '.rst' != entry.name.sub(entry.name.length - 4))
				continue;

			if(skipDocs.contains(entry.name.sub(0, entry.name.length - 4).toString()))
				continue;

			var path = Path.join([docsDir, entry.name.toString()]);
			Sys.println('Generating ' + entry.name.sub(0, entry.name.length - 4).toString() + '...');

			var lines = File.getContent(path).split('\n');
			var totalLines = lines.length;
			while(lines.length > 0) {
				var line = lines.shift().trim();
				var lineNumber = totalLines - lines.length;
				try {
					if(line.startsWith('.. c:function:: ')) {
						var sig = parseFunction(line.substr('.. c:function:: '.length).trim());
						hxFile.writeString(hxFunctionBinding(sig));
					}
				} catch(e:Skip) {
					continue;
				} catch(e) {
					Sys.stderr().writeString('Error on symbol at line: $lineNumber\n${e.details()}\n');
					Sys.exit(1);
				}
			}
		}
		hxFile.writeString('}\n\n');
		hxFile.writeString(generateHXTypes(hxTypesToGenerate));
		hxFile.close();
		scan.end();
	}

	static function collectPredefinedHxTypesToMap(hxCode:String, map:Map<String,String>) {
		var re = ~/(abstract|typedef|class)\s+([^ (<]+)/;
		var pos = 0;
		while(re.matchSub(hxCode, pos)) {
			map.set(re.matched(2), re.matched(1));
			var p = re.matchedPos();
			pos = p.pos + p.len;
		}
	}

	static function generateHXTypes(types:Map<String,String>):String {
		var lines = [];
		for(hx => c in types) {
			lines.push('@:native("$c")');
			lines.push('extern class $hx {');
			lines.push('	@:native("new $c") public static function create():Star<$hx>;');
			lines.push('}');
		}
		return lines.join('\n');
	}

	static function rootDir(?p:PosInfos):String {
		var generatorPath = FileSync.realPath(p.fileName).resolve().toString();
		return new Path(new Path(new Path(generatorPath).dir).dir).dir;
	}

	static function parseFunction(str:String):FunctionSignature {
		str = str.substr(0, str.length - (str.endsWith(';') ? 2 : 1));
		var parts = str.split('(');
		var returnAndName = parseTypeAndName(parts[0]);
		var args = parts[1].split(',').map(StringTools.trim);
		return {
			name: returnAndName.name,
			returnType: returnAndName.type,
			arguments: args.map(parseTypeAndName)
		}
	}

	static function parseType(str:String):CType {
		var result = {
			name:null,
			stars:0,
			const:false,
			unsigned:false,
			struct:false
		}
		var parts = str.split(' ');
		result.name = parts.pop();
		for(s in parts) {
			switch s {
				case 'const': result.const = true;
				case 'unsigned': result.unsigned = true;
				case 'struct': result.struct = true;
				case _: throw 'Unknown type modifier: $s';
			}
		}
		while(result.name.endsWith('*')) {
			result.stars++;
			result.name = result.name.substr(0, result.name.length - 1);
		}
		return result;
	}

	static var reArray = ~/\[([0-9]*)\]$/;

	static function parseTypeAndName(str:String):TypeAndName {
		var result = {
			type:null,
			name:'',
			array:-1
		}
		var spacePos = str.lastIndexOf(' ');
		if(spacePos < 0) {
			result.type = parseType(str);
		} else {
			result.type = parseType(str.substring(0, spacePos));
			result.name = str.substr(spacePos + 1);
		}
		if(reArray.match(result.name)) {
			var p = reArray.matchedPos();
			result.name = result.name.substring(0, p.pos);
			result.array = switch(reArray.matched(1)) {
				case '': 0;
				case n: Std.parseInt(n);
			}
		}
		return result;
	}

	static function isUvBuf(cType:String):Bool {
		return cType == 'uv_buf_t' || cType == 'const uv_buf_t';
	}

	static final reCapitalize = ~/_[a-z]/g;

	static function snakeToPascalCase(str:String):String {
		return str.charAt(0).toUpperCase() + reCapitalize.map(str.substr(1), r -> r.matched(0).replace('_', '').toUpperCase());
	}

	static function mapHXType(type:CType):String {
		var name = switch type.name {
			case 'void': 'Void';
			case 'char' if(type.stars == 1 && type.const): return 'ConstCharStar';
			case 'char': 'Char';
			case 'int': 'Int';
			case 'double': 'Float';
			case '...': 'Rest<Any>';
			case 'size_t': 'SizeT';
			case 'ssize_t': 'SSizeT';
			case 'int64_t': 'Int64';
			case 'uint64_t': 'UInt64';
			case 'FILE': 'FILE';
			case _:
				var hxType = snakeToPascalCase(type.name);
				if(!predefinedHxTypes.exists(hxType))
					hxTypesToGenerate.set(hxType, type.name);
				hxType;
		}
		for(i in 0...type.stars)
			name = 'Star<$name>';
		return name;
	}

	static function functionName(name:String):String {
		return if(name.startsWith('uv_'))
			name.substr('uv_'.length)
		else
			throw 'Function name is expected to start with "uv_": "$name"';
	}

	static function hxFunctionBinding(sig:FunctionSignature):String {
		function compose(name:String, args:Array<String>) {
			var args = args.join(', ');
			var ret = mapHXType(sig.returnType);
			return '\t@:native("${sig.name}") static function $name($args):$ret;\n';
		}
		function mapArg(a:TypeAndName):String {
			var type = mapHXType(a.type);
			if(a.array >= 0)
				type = 'Reference<$type>';
			var name = a.name;
			if(name == '') {
				if(a.type.name.startsWith('uv_')) {
					name = a.type.name.substr('uv_'.length);
					if(name.endsWith('_t'))
						name = name.substring(0, name.length - '_t'.length);
				} else {
					name = 'v';
				}
			}
			return '$name:$type';
		}
		function isNotVoid(a:TypeAndName):Bool {
			return !(a.type.name == 'void' && a.type.stars == 0);
		}
		var name = functionName(sig.name);
		var returnType = mapHXType(sig.returnType);
		var args = sig.arguments.filter(isNotVoid).map(mapArg).join(', ');
		return '\t@:native("${sig.name}") static function $name($args):$returnType;\n';
	}
}

