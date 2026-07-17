import json
import random
import string
import os

def generate_random_string(length):
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))

def generate_large_json(filename, target_size_mb):
    target_bytes = target_size_mb * 1024 * 1024
    
    # Pre-generate some realistic-ish structures
    templates = [
        {"id": 0, "name": "user_{}", "email": "user_{}@example.com", "active": True, "metadata": {"role": "admin", "score": 95}},
        {"id": 0, "name": "customer_{}", "email": "customer_{}@store.com", "active": False, "metadata": {"role": "guest", "score": 10}},
        {"id": 0, "name": "system_{}", "email": "sys_{}@internal.net", "active": True, "metadata": {"role": "service", "score": 100}}
    ]
    
    current_size = 0
    batch_size = 100000
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write('[\n')
        
        counter = 0
        first = True
        
        while current_size < target_bytes:
            batch = []
            for _ in range(batch_size):
                t = templates[random.randint(0, len(templates)-1)].copy()
                t["id"] = counter
                t["name"] = t["name"].format(counter)
                t["email"] = t["email"].format(counter)
                
                # Add some random highly-compressible payload
                t["payload"] = generate_random_string(50)
                batch.append(json.dumps(t))
                counter += 1
                
            chunk = ""
            if not first:
                chunk += ",\n"
            first = False
            
            chunk += ",\n".join(batch)
            f.write(chunk)
            current_size = os.path.getsize(filename)
            print(f"Generated {current_size / (1024*1024):.2f} MB...", end='\r')
            
        f.write('\n]\n')
    
    print(f"\nDone! Final size: {os.path.getsize(filename) / (1024*1024):.2f} MB")

if __name__ == "__main__":
    generate_large_json("large_test.json", 500)
