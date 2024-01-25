#ifndef JSON_H__
#define JSON_H__
#define EMPTY 0
#define qKey(x) (String('"') + x + "\":")

#define NUMERIC true
namespace JSON {
  String payload;
  String read(String, String);
  String JSON(String = "{}");
  bool hasKey(String&, String);

  bool hasKey(String key) {
    return hasKey(JSON::payload, key);
  }
  bool hasKey(String& json, String key) {
    if (json.indexOf(qKey(key)) > -1) {
      return true;
    }
    return false;
  }
  template<typename T>
  void add(String& json, String key, T __value, bool nonString = false) {
    if (JSON::hasKey(json, key)) {
      String val = JSON::read(json, key);
      json.replace(qKey(key) + '"' + val + '"', qKey(key) + '"' + String(__value) + '"');
      json.replace(qKey(key) + val, qKey(key) + String(__value));
    } else {
      json += "~";
      if (nonString) {
        json.replace("}~", String(',') + qKey(key) + String(__value) + '}');
      } else {
        json.replace("}~", String(',') + qKey(key) + '"' + String(__value) + "\"}");
      }
      json.replace("{,", "{");
    }
  }


  String JSON(String json) {
    return json;
  }
  String read(String key) {
    return read(JSON::payload, key);
  }
  String read(String json, String key) {
    if (JSON::hasKey(json, key)) {
      int index = json.indexOf(qKey(key));
      index += qKey(key).length();
      if (isDigit(json[index])) {
        float response;
        sscanf(json.substring(index).c_str(), "%f", &response);
        return String(response);
      }
      if (json[index] == '"') {
        index++;
        json = json.substring(index);
        return json.substring(0, json.indexOf('"'));
      }
      char startCharacter = json[index];
      char endCharacter;
      if (startCharacter == '{') endCharacter = '}';
      if (startCharacter == '[') endCharacter = ']';
      int stack = 0;
      int startIndex = index;
      for (int i = index; i < json.length(); i++) {
        if (json[i] == startCharacter) {
          stack++;
        } else if (json[i] == endCharacter) {
          stack--;
        }
        if (stack == EMPTY) {
          return json.substring(startIndex, i + 1);
        }
      }
      return json.substring(index);
    }
    return "";
  }


  void prettify(String& json) {
    int indentLevel = 0;
    bool inString = false;

    for (size_t i = 0; i < json.length(); i++) {
      char c = json[i];

      if (c == '"') {
        inString = !inString;
      }

      if (!inString) {
        if (c == '{' || c == '[') {
          Serial.print(c);
          Serial.print('\n');
          indentLevel++;

          for (int i = 0; i < indentLevel * 2; i++) {
            Serial.print("  ");
          }
        } else if (c == '}' || c == ']') {
          Serial.println();
          indentLevel--;

          for (int i = 0; i < indentLevel * 2; i++) {
            Serial.print("  ");
          }

          Serial.print(c);
        } else if (c == ',') {
          Serial.print(c);
          Serial.print('\n');

          for (int i = 0; i < indentLevel * 2; i++) {
            Serial.print("  ");
          }
        } else {
          Serial.print(c);
        }
      } else {
        Serial.print(c);
      }
    }
    Serial.println();
  }

  void prettify(String title, String& json) {
    Serial.print(title + " : ");
    prettify(json);
  }

};
#endif
