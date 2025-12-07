
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