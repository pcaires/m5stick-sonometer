


void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\r\n", path);
  if(fs.remove(path)){
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}


void listDir(fs::FS &fs){
    Serial.printf("Listing directory\n");

    File root = fs.open("/");
    File file = root.openNextFile();
    StickCP2.Display.clear();
    StickCP2.Display.setFont(&fonts::Font0);
    StickCP2.Display.setCursor(10, 10);  // Position the text
    while(file){
        StickCP2.Display.print(file.name());
        StickCP2.Display.print("    SIZE: ");
        StickCP2.Display.print(file.size());
        Serial.print("  FILE: ");
        Serial.print(file.name());
        auto y = StickCP2.Display.getCursorY();
        StickCP2.Display.setCursor(10, y+10);  // Position next text
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
        file = root.openNextFile();
    }
    StickCP2.Display.setFont(&fonts::FreeSansBoldOblique12pt7b);
}

void format(fs::FS &fs){
    Serial.printf("Deleting all files\n");
    File root = fs.open("/");
    auto file = root.getNextFileName();
    while(file != ""){
        root.close();

        Serial.printf("Deleting file: %s\r\n", file);
        if(fs.remove(file)){
          Serial.println("- file deleted");
        } else {
          Serial.println("- delete failed");
        }
        
        root = fs.open("/");
        file = root.getNextFileName();
    }
    root.close();
}