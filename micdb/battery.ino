

void dispBat(){
  auto bat = StickCP2.Power.getBatteryLevel();
  StickCP2.Display.fillRect(5,115,90,15,TFT_RED);
  StickCP2.Display.setFont(&fonts::Font0);
  StickCP2.Display.setCursor(10, 120);  // Position the text
  StickCP2.Display.printf("Battery: %d",bat);
}