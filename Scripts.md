Run this in tools/run to extract defines

```bash
grep -hoir --include="*.hx" m*defines.\\w*\(\"\\w*\" . | grep -oi \"\\w*\" | grep -oi "\w\+" | sort | uniq 
```
