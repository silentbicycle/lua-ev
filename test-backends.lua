require "evc"

print("Supported: ")
for k,v in pairs(evc.supported_backends()) do print("", k,v) end
print("Recommended: ")
for k,v in pairs(evc.recommended_backends()) do print("", k,v) end
print("Embeddable: ")
for k,v in pairs(evc.embeddable_backends()) do print("", k,v) end
