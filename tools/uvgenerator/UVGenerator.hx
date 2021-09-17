import haxe.Exception;
import sys.io.File;
import eval.luv.File.FileSync;
import haxe.io.Path;
import haxe.PosInfos;
import eval.luv.Dir.DirSync;

using StringTools;

typedef CType = {
	var name:String;
	var stars:Int;
	var const:Bool;
	var unsigned:Bool;
	var struct:Bool;
}

typedef CName = {
	var label:String;
	var array:Int;
}

typedef TypeAndName = {
	var type:CType;
	var name:Null<CName>;
}

typedef FunctionSignature = {
	var name:String;
	var returnType:CType;
	var args:Array<TypeAndName>;
}

typedef StructSignature = {
	var type:CType;
	var name:CName;
	var fields:Array<TypeAndName>;
	var isUnion:Bool;
}

typedef CallbackSignature = {
	var name:String;
	var args:Array<TypeAndName>;
	var returnType:CType;
}

typedef EnumSignature = {
	var name:CName;
	var constructors:Array<String>;
}

enum TypeKind {
	UnknownType(cName:String);
	CallbackType(sig:CallbackSignature);
	StructType(sig:StructSignature);
	EnumType(sig:EnumSignature);
}

class Skip extends Exception {}

class UVGenerator {
	static inline var DOCS = 'project/thirdparty/libuv-1.42.0/docs/src';
	static inline var UV_HX = 'tools/uvgenerator/UV.hx';
	static inline var UV_HX_HEADER = 'tools/uvgenerator/UV.hx.header';

	static final skipDocs = ['api', 'dll', 'guide', 'index', 'migration_010_100',
		'poll', 'threading', 'threadpool', 'upgrading'];

	static final predefinedHxTypes = new Map<String,String>();
	static final hxTypesToGenerate = new Map<String,TypeKind>();

	static function main() {
		var root = rootDir();
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
					} else if(line.startsWith('.. c:type:: ') && erCallback.match(line)) {
						var sig = parseCallback(line.substr('.. c:type:: '.length));
						var hxName = snakeToPascalCase(sig.name);
						if(!predefinedHxTypes.exists(hxName))
							hxTypesToGenerate.set(hxName, CallbackType(sig));
					} else if(line.startsWith('typedef struct ')) {
						var structs = parseStruct(line, lines);
						for(sig in structs) {
							var hxName = snakeToPascalCase(sig.type.name);
							if(!predefinedHxTypes.exists(hxName))
								hxTypesToGenerate.set(hxName, StructType(sig));
						}
					} else if(reEnum.match(line)) {
						var sig = parseEnum(line, lines);
						var hxName = snakeToPascalCase(sig.name.label);
						if(!predefinedHxTypes.exists(hxName))
								hxTypesToGenerate.set(hxName, EnumType(sig));
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

	static function generateHXTypes(types:Map<String,TypeKind>):String {
		var lines = [];
		for(hxName => kind in types) {
			lines.push('');
			switch kind {
				case UnknownType(cName):
					lines.push('@:native("$cName")');
					lines.push('@:structAccess extern class $hxName {');
					lines.push('	@:native("new $cName") public static function create():RawPointer<$hxName>;');
					lines.push('}');
				case CallbackType(sig):
					lines.push('typedef $hxName = Callable<(${generateHXArgs(sig.args)})->${mapHXType(sig.returnType)}>');
				case StructType(sig):
					if(!sig.isUnion)
						lines.push('@:native("${sig.type.name}")');
					lines.push('@:structAccess extern class $hxName {');
					if(!sig.isUnion) {
						lines.push('	function new():Void;');
						lines.push('	@:native("new ${sig.type.name}") static function create():RawPointer<$hxName>;');
					}
					lines.push(generateHXFields(sig.fields));
					lines.push('}');
				case EnumType(sig):
					lines.push('extern enum abstract $hxName(Int) to Int {');
					for(ctor in sig.constructors)
						lines.push('	@:native("$ctor") var $ctor;');
					lines.push('}');
			}
		}
		return lines.join('\n');
	}

	static function rootDir(?p:PosInfos):String {
		var generatorPath = FileSync.realPath(p.fileName).resolve().toString();
		return new Path(new Path(new Path(generatorPath).dir).dir).dir;
	}

	static function stripComments(line:String, lines:Array<String>):String {
		var commentPos = line.indexOf('//');
		if(commentPos >= 0)
			line = line.substr(0, commentPos);
		commentPos = line.indexOf('/*');
		if(commentPos >= 0) {
			var commentEnd = line.indexOf('*/');
			if(commentEnd >= 0) {
				line = line.substr(0, commentPos) + line.substr(commentEnd + 2);
			} else {
				line = line.substring(0, commentPos);
				while(lines.length > 0) {
					commentEnd = lines[0].indexOf('*/');
					if(commentEnd < 0) {
						lines.shift();
					} else {
						lines[0] = lines[0].substr(commentEnd + 2);
						break;
					}
				}
			}
		}
		line = line.trim();
		while(lines.length > 0 && line == '')
			line = lines.shift().trim();
		return line;
	}

	static final reEnum = ~/^(typedef )?enum\s+(.*?)\s*\{$/;

	static function parseEnum(firstLine:String, lines:Array<String>):EnumSignature {
		if(!reEnum.match(firstLine))
			throw 'Unexpected enum declaration: "$firstLine"';
		var isTypedef = reEnum.matched(1) == 'typedef ';
		var result = {
			name: switch reEnum.matched(2) {
				case null | '': null;
				case str: parseName(str);
			},
			constructors: []
		}
		while(lines.length > 0) {
			var line = stripComments(lines.shift(), lines);
			//strip values and commas
			var eqPos = line.indexOf('=');
			if(eqPos >= 0)
				line = line.substring(0, eqPos)
			else if(line.endsWith(','))
				line = line.substr(0, line.length - 1);
			line = line.trim();

			if(line.startsWith('}')) {
				if(isTypedef) {
					line = line.endsWith(';') ? line.substring(1, line.length - 1) : line.substr(1);
					result.name = parseName(line.trim());
				}
				break;
			} else
				result.constructors.push(line);
		}
		return result;
	}

	static final reStructTypeName = ~/(struct\s+(.+?))\s*\{/;

	static function parseStruct(firstLine:String, lines:Array<String>, root = true):Array<StructSignature> {
		var result = [];
		var fields = [];
		var unions = [];
		var type = !root && reStructTypeName.match(firstLine) ? parseType(reStructTypeName.matched(1)) : null;
		var name = null;
		while(lines.length > 0) {
			var line = stripComments(lines.shift(), lines);
			line = line.trim();
			if(line.startsWith('union {')) {
				var sub = parseStruct('struct UNION_NAME_PLACEHOLDER {', lines, false);
				if(sub.length > 0) {
					sub[0].isUnion = true;
					result = result.concat(sub);
					fields.push({name:sub[0].name, type:sub[0].type});
					unions.push(sub[0]);
				}
			} else if(line.startsWith('struct ') && line.endsWith('{')) {
				var sub = parseStruct(line, lines, false);
				if(sub.length > 0) {
					result = result.concat(sub);
					fields.push({name:sub[0].name, type:sub[0].type});
				}
			} else {
				var splitPos = line.lastIndexOf(' ');
				if(line.endsWith(';'))
					line = line.substr(0, line.length - 1);
				if(line.startsWith('}')) {
					name = parseName(line.substr(splitPos + 1).trim());
					break;
				} else {
					fields.push(parseTypeAndName(line));
				}
			}
		}
		if(name == null)
			throw 'Unexpected struct without a name';
		for (union in unions)
			union.type.name = '${name.label}_${union.name.label}_union';
		if(type == null) {
			type = {
				name: name.label,
				stars:0,
				const:false,
				unsigned:false,
				struct:true
			}
		}
		result.unshift({
			type: type,
			name: name,
			fields: fields,
			isUnion: false
		});
		return result;
	}

	static final erCallback = ~/(.+?)\s*\(\*([a-z_]+)\)\((.*?)\)/;

	static function parseCallback(str:String):CallbackSignature {
		if(!erCallback.match(str))
			throw 'Unknown callback signature format: $str';
		return {
			name: erCallback.matched(2),
			args: erCallback.matched(3).split(', ').map(parseTypeAndName),
			returnType: parseType(erCallback.matched(1).trim())
		}
	}

	static function parseFunction(str:String):FunctionSignature {
		str = str.substr(0, str.length - (str.endsWith(';') ? 2 : 1));
		var parts = str.split('(');
		var returnAndName = parseTypeAndName(parts[0]);
		var args = parts[1].split(',').map(StringTools.trim);
		return {
			name: returnAndName.name.label,
			returnType: returnAndName.type,
			args: args.map(parseTypeAndName)
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
		for(s in parts) {
			switch s {
				case 'const': result.const = true;
				case 'unsigned': result.unsigned = true;
				case 'struct': result.struct = true;
				case _ if(result.name != null): throw 'Unknown type modifier: "$s"';
				case _: result.name = s;
			}
		}
		if(result.name == null)
			result.name = 'int';
		while(result.name.endsWith('*')) {
			result.stars++;
			result.name = result.name.substr(0, result.name.length - 1);
		}
		if(result.name == '')
			result.name = 'int';
		return result;
	}

	static final reArray = ~/\[([0-9]*)\]$/;

	static function parseName(str:String):CName {
		var result = {
			label:str,
			array:-1
		}
		if(reArray.match(result.label)) {
			var p = reArray.matchedPos();
			result.label = result.label.substring(0, p.pos);
			result.array = switch(reArray.matched(1)) {
				case '': 0;
				case n: Std.parseInt(n);
			}
		}
		return result;
	}

	static function parseTypeAndName(str:String):TypeAndName {
		var result = {
			type:null,
			name: null
		}
		var spacePos = str.lastIndexOf(' ');
		if(spacePos < 0) {
			result.type = parseType(str);
		} else {
			result.type = parseType(str.substring(0, spacePos));
			result.name = parseName(str.substr(spacePos + 1));
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
			case 'void' if(type.stars > 0): 'cpp.Void';
			case 'void': 'Void';
			case 'char' if(type.stars == 1 && type.const): return 'ConstCharStar';
			case 'char': 'Char';
			case 'int' if(type.unsigned): 'UInt';
			case 'int': 'Int';
			case 'double': 'Float';
			case '...': 'Rest<Any>';
			case 'size_t': 'SizeT';
			case 'ssize_t': 'SSizeT';
			case 'int32_t': 'Int32';
			case 'int64_t': 'Int64';
			case 'uint64_t': 'UInt64';
			case 'long': 'Int64'; // TODO: is this OK?
			case 'FILE': 'FILE';
			case _:
				var hxType = snakeToPascalCase(type.name);
				if(!predefinedHxTypes.exists(hxType) && !hxTypesToGenerate.exists(hxType))
					hxTypesToGenerate.set(hxType, UnknownType(type.name));
				hxType;
		}
		for(i in 0...type.stars)
			name = 'RawPointer<$name>';
		return name;
	}

	static function functionName(name:String):String {
		return if(name.startsWith('uv_'))
			name.substr('uv_'.length)
		else
			throw 'Function name is expected to start with "uv_": "$name"';
	}

	static function generateHXArgs(args:Array<TypeAndName>):String {
		return generateHXList(args, '', '', ', ');
	}

	static function generateHXFields(fields:Array<TypeAndName>):String {
		return generateHXList(fields, '\tvar ', ';', '\n');
	}

	static function generateHXList(list:Array<TypeAndName>, prefix:String, postfix:String, separator:String):String {
		function isNotVoid(a:TypeAndName):Bool {
			return !(a.type.name == 'void' && a.type.stars == 0);
		}
		var i = 0;
		function mapArg(a:TypeAndName):String {
			var type = mapHXType(a.type);
			var name = null;
			if(a.name != null) {
				name = a.name.label;
				if(a.name.array >= 0)
					type = 'Reference<$type>';
			} else {
				if(a.type.name.startsWith('uv_')) {
					name = a.type.name.substr('uv_'.length);
					if(name.endsWith('_t'))
						name = name.substring(0, name.length - '_t'.length);
				} else {
					name = i == 0 ? 'v' : 'v$i';
					i++;
				}
			}
			return '$prefix$name:$type$postfix';
		}
		return list.filter(isNotVoid).map(mapArg).join(separator);
	}

	static function hxFunctionBinding(sig:FunctionSignature):String {
		function compose(name:String, args:Array<String>) {
			var args = args.join(', ');
			var ret = mapHXType(sig.returnType);
			return '\t@:native("${sig.name}") static function $name($args):$ret;\n';
		}

		function isNotVoid(a:TypeAndName):Bool {
			return !(a.type.name == 'void' && a.type.stars == 0);
		}
		var name = functionName(sig.name);
		var returnType = mapHXType(sig.returnType);
		var args = generateHXArgs(sig.args);
		return '\t@:native("${sig.name}") static function $name($args):$returnType;\n';
	}
}

