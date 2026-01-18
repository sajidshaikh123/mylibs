
bool resizeJsonDocument(DynamicJsonDocument *&doc, size_t newSize) {
  size_t json_size = 0;
  if (doc == nullptr) {
    // Allocate if not created yet
    doc = new DynamicJsonDocument(newSize);
    Serial.print("Allocated new JSON document with size: ");
    Serial.println(newSize);
    return true;
  }else{
      json_size = doc->memoryUsage();
      Serial.print(" OLD JSON size: ");
      Serial.println(json_size);
  }


  // Allocate new larger document
  DynamicJsonDocument *newDoc = new DynamicJsonDocument(json_size + newSize);
  if (!newDoc) {
    Serial.println("Failed to allocate larger JSON document");
    return false;
  }

  // Copy existing content into the new document
  newDoc->set(*doc);

  // Delete old document and replace with new one
  delete doc;
  doc = newDoc;

  Serial.print(" Resized JSON document to: ");
  Serial.println(newSize + json_size);
  return true;
}

// Compare two JSON objects for equality (fields and values)
// Returns true if both objects have same keys with same values
bool compareJsonObjects(const DynamicJsonDocument &json1, const DynamicJsonDocument &json2, bool verbose = false)
{
    // Check if both are objects
    if (!json1.is<JsonObject>() || !json2.is<JsonObject>())
    {
        if (verbose) Serial.println("[COMPARE] One or both JSON documents are not objects");
        return false;
    }

    JsonObjectConst obj1 = json1.as<JsonObjectConst>();
    JsonObjectConst obj2 = json2.as<JsonObjectConst>();

    // Check if sizes match
    if (obj1.size() != obj2.size())
    {
        if (verbose) 
        {
            Serial.printf("[COMPARE] Size mismatch: obj1=%d, obj2=%d\n", obj1.size(), obj2.size());
        }
        return false;
    }

    // Check if all keys in obj1 exist in obj2 with same values
    for (JsonPairConst kv1 : obj1)
    {
        const char* key = kv1.key().c_str();
        
        // Check if key exists in obj2
        if (!obj2.containsKey(key))
        {
            if (verbose)
            {
                Serial.printf("[COMPARE] Key '%s' exists in obj1 but not in obj2\n", key);
            }
            return false;
        }

        JsonVariantConst val1 = kv1.value();
        JsonVariantConst val2 = obj2[key];

        // Compare values based on type
        if (val1.is<int>() && val2.is<int>())
        {
            if (val1.as<int>() != val2.as<int>())
            {
                if (verbose)
                {
                    Serial.printf("[COMPARE] Key '%s': int value mismatch (%d != %d)\n", 
                                  key, val1.as<int>(), val2.as<int>());
                }
                return false;
            }
        }
        else if (val1.is<float>() && val2.is<float>())
        {
            // For float comparison, use small epsilon for tolerance
            float diff = abs(val1.as<float>() - val2.as<float>());
            if (diff > 0.0001)
            {
                if (verbose)
                {
                    Serial.printf("[COMPARE] Key '%s': float value mismatch (%.4f != %.4f)\n", 
                                  key, val1.as<float>(), val2.as<float>());
                }
                return false;
            }
        }
        else if (val1.is<const char*>() && val2.is<const char*>())
        {
            if (strcmp(val1.as<const char*>(), val2.as<const char*>()) != 0)
            {
                if (verbose)
                {
                    Serial.printf("[COMPARE] Key '%s': string value mismatch ('%s' != '%s')\n", 
                                  key, val1.as<const char*>(), val2.as<const char*>());
                }
                return false;
            }
        }
        else if (val1.is<bool>() && val2.is<bool>())
        {
            if (val1.as<bool>() != val2.as<bool>())
            {
                if (verbose)
                {
                    Serial.printf("[COMPARE] Key '%s': bool value mismatch (%d != %d)\n", 
                                  key, val1.as<bool>(), val2.as<bool>());
                }
                return false;
            }
        }
        else if (val1.is<JsonObject>() && val2.is<JsonObject>())
        {
            // Recursive comparison for nested objects
            DynamicJsonDocument nestedDoc1(512);
            DynamicJsonDocument nestedDoc2(512);
            nestedDoc1.set(val1);
            nestedDoc2.set(val2);
            
            if (!compareJsonObjects(nestedDoc1, nestedDoc2, verbose))
            {
                if (verbose)
                {
                    Serial.printf("[COMPARE] Key '%s': nested object mismatch\n", key);
                }
                return false;
            }
        }
        else if (val1.is<JsonArray>() && val2.is<JsonArray>())
        {
            JsonArrayConst arr1 = val1.as<JsonArrayConst>();
            JsonArrayConst arr2 = val2.as<JsonArrayConst>();
            
            if (arr1.size() != arr2.size())
            {
                if (verbose)
                {
                    Serial.printf("[COMPARE] Key '%s': array size mismatch (%d != %d)\n", 
                                  key, arr1.size(), arr2.size());
                }
                return false;
            }
            
            // Compare array elements
            size_t idx = 0;
            for (JsonVariantConst v1 : arr1)
            {
                JsonVariantConst v2 = arr2[idx];
                
                // Simple comparison (can be extended for nested arrays/objects)
                String str1, str2;
                serializeJson(v1, str1);
                serializeJson(v2, str2);
                
                if (str1 != str2)
                {
                    if (verbose)
                    {
                        Serial.printf("[COMPARE] Key '%s': array element[%d] mismatch\n", key, idx);
                    }
                    return false;
                }
                idx++;
            }
        }
        else if (val1.isNull() && val2.isNull())
        {
            // Both are null, continue
            continue;
        }
        else
        {
            // Type mismatch
            if (verbose)
            {
                Serial.printf("[COMPARE] Key '%s': type mismatch\n", key);
            }
            return false;
        }
    }

    if (verbose)
    {
        Serial.println("[COMPARE] JSON objects are equal");
    }
    
    return true;
}

// Simpler version - compares by serializing to string (less precise but faster)
bool compareJsonObjectsSimple(const DynamicJsonDocument &json1, const DynamicJsonDocument &json2)
{
    String str1, str2;
    serializeJson(json1, str1);
    serializeJson(json2, str2);
    
    return (str1 == str2);
}

// Compare only specific fields
bool compareJsonFields(const DynamicJsonDocument &json1, const DynamicJsonDocument &json2, 
                       const char* fields[], int fieldCount, bool verbose = false)
{
    JsonObjectConst obj1 = json1.as<JsonObjectConst>();
    JsonObjectConst obj2 = json2.as<JsonObjectConst>();

    for (int i = 0; i < fieldCount; i++)
    {
        const char* field = fields[i];
        
        // Check if field exists in both
        if (!obj1.containsKey(field) || !obj2.containsKey(field))
        {
            if (verbose)
            {
                Serial.printf("[COMPARE] Field '%s' missing in one of the objects\n", field);
            }
            return false;
        }

        // Compare values
        String val1Str, val2Str;
        serializeJson(obj1[field], val1Str);
        serializeJson(obj2[field], val2Str);
        
        if (val1Str != val2Str)
        {
            if (verbose)
            {
                Serial.printf("[COMPARE] Field '%s' mismatch: '%s' != '%s'\n", 
                              field, val1Str.c_str(), val2Str.c_str());
            }
            return false;
        }
    }

    return true;
}

// Test function demonstrating usage
void testJsonComparison()
{
    Serial.println("\n=== TESTING: JSON Comparison ===");

    // Test 1: Equal objects
    DynamicJsonDocument json1(512);
    JsonObject obj1 = json1.to<JsonObject>();
    obj1["ps_no"] = "1234";
    obj1["cavities"] = 16;
    obj1["cycle_time"] = 500;
    obj1["loading_time"] = 300;

    DynamicJsonDocument json2(512);
    JsonObject obj2 = json2.to<JsonObject>();
    obj2["ps_no"] = "1234";
    obj2["cavities"] = 16;
    obj2["cycle_time"] = 500;
    obj2["loading_time"] = 300;

    Serial.println("Test 1: Equal objects");
    bool result1 = compareJsonObjects(json1, json2, true);
    Serial.printf("Result: %s\n\n", result1 ? "EQUAL" : "NOT EQUAL");

    // Test 2: Different values
    DynamicJsonDocument json3(512);
    JsonObject obj3 = json3.to<JsonObject>();
    obj3["ps_no"] = "1234";
    obj3["cavities"] = 16;
    obj3["cycle_time"] = 510;  // Different value
    obj3["loading_time"] = 300;

    Serial.println("Test 2: Different values");
    bool result2 = compareJsonObjects(json1, json3, true);
    Serial.printf("Result: %s\n\n", result2 ? "EQUAL" : "NOT EQUAL");

    // Test 3: Missing field
    DynamicJsonDocument json4(512);
    JsonObject obj4 = json4.to<JsonObject>();
    obj4["ps_no"] = "1234";
    obj4["cavities"] = 16;
    obj4["cycle_time"] = 500;
    // loading_time missing

    Serial.println("Test 3: Missing field");
    bool result3 = compareJsonObjects(json1, json4, true);
    Serial.printf("Result: %s\n\n", result3 ? "EQUAL" : "NOT EQUAL");

    // Test 4: Compare specific fields only
    const char* fieldsToCompare[] = {"ps_no", "cavities"};
    Serial.println("Test 4: Compare specific fields only (ps_no, cavities)");
    bool result4 = compareJsonFields(json1, json3, fieldsToCompare, 2, true);
    Serial.printf("Result: %s\n\n", result4 ? "EQUAL" : "NOT EQUAL");

    // Test 5: Simple string comparison
    Serial.println("Test 5: Simple string comparison");
    bool result5 = compareJsonObjectsSimple(json1, json2);
    Serial.printf("Result: %s\n", result5 ? "EQUAL" : "NOT EQUAL");

    Serial.println("=== Test Complete ===");
}