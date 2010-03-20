-- Generate C code for a perfect hash for a list of tags.
-- This is too simple for general use, but quite good enough
-- for the small lists of tags used in libev.

local byte = string.byte
local function printf(...) print(string.format(...)) end

function gen_perfect_hashes(ts)
   local minlen = math.huge
   for _,tag in ipairs(ts) do
      minlen = math.min(minlen, tag:len())
   end
   for first=1,minlen do
      for second=first+1,minlen do
         local bytes = { first, second }
         for mul=1,100,2 do
            local function hash(tag)
               local v = 0
               for i=1,#bytes do v = mul*v + byte(tag, bytes[i]) end
               return v
            end

            local ok, hs = true, {}
            for _,tag in ipairs(ts) do
               local h = hash(tag)
               if hs[h] then ok = false; break else hs[h] = tag end
            end
            if ok then return hs, mul, bytes end
         end
      end
   end
end

function gen_c(prefix, hs, mul, bytes)
   printf("\tint hash = %d*tag[%d] + tag[%d];",
          mul, bytes[1] - 1, bytes[2] - 1)
   printf('\tif (DEBUG) printf("tag: %%s, hash: %%d\\n", tag, hash);');
   printf("\tswitch (hash) {")
   for hash,tag in pairs(hs) do
      printf("\t\tcase %d: return %s%s; break;",
             hash, prefix, tag:upper())
   end
   printf('\t\tdefault: printf("match failed: %%d (%%s)", hash, tag); return -1;')
   printf("\t}")
end

if #arg == 0 then
   print "#error Usage: gen_perfhash.lua list of tags"
else
   local prefix = arg[1]
   table.remove(arg, 1)
   local hs, mul, bytes = gen_perfect_hashes(arg)
   if hs then gen_c(prefix, hs, mul, bytes) else
      print '#error Perfect hash not found'
   end
end
