/*******************************************************************
Creative Commons Attribution-NonCommercial License

Copyright © 2020-2025 Nathan Woelfle

This work is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License.

You are free to:

    Share — copy and redistribute the material in any medium or format
    Adapt — remix, transform, and build upon the material

Under the following conditions:

    Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
    NonCommercial — You may not use the material for commercial purposes.
    No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.

Notices:

    You may not use this work for commercial purposes without written permission from the copyright holder.
    This work is provided "as is" without warranty of any kind, either express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and noninfringement. In no event shall the authors or copyright holders be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

Scope:

    This license applies to both the hardware and software components of the DarkLight Cover Calibrator.

Modified Versions:

    You are permitted to create modified versions of the DarkLight Cover Calibrator for non-commercial use, provided that you:
        Retain the original copyright notice and license terms.
        Include a clear reference to the original creator (Nathan Woelfle) and provide a link to the original work.

Jurisdiction:

    This license is governed by the laws of the United States of America, and by international copyright laws and treaties.

For more information, please refer to the full terms of the Creative Commons Attribution-NonCommercial 4.0 International License: https://creativecommons.org/licenses/by-nc/4.0/
*******************************************************************/

#include "config.h"
#include "darklight_covercalibrator.h"
#include "indicom.h"
#include "connectionplugins/connectionserial.h"
#include <termios.h>
#include <mutex>

static std::unique_ptr<DarkLight_CoverCalibrator> mydriver(new DarkLight_CoverCalibrator());
std::mutex serialMutex;

DarkLight_CoverCalibrator::DarkLight_CoverCalibrator() : lightDisabled(false), coverIsMoving(false), lightIsReady(true),
    autoOn(false), autoHeatOn(false), heatOnClose(false), heatModeIsChanging(false)
{
    setVersion(CDRIVER_VERSION_MAJOR, CDRIVER_VERSION_MINOR);
}

const char *DarkLight_CoverCalibrator::getDefaultName()
{
    return "DarkLight Cover Calibrator";
}

bool DarkLight_CoverCalibrator::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);
    //save the following values
    StabilizeTimeNP.save(fp);
    AutoOnSP.save(fp);
    DisableLightSP.save(fp);
    AutoHeatOnSP.save(fp);
    HeatOnCloseSP.save(fp);

    return true;
}

bool DarkLight_CoverCalibrator::initProperties()
{
    //initialize the parent's properties first
    INDI::DefaultDevice::initProperties();

    //add serial connection
    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
    {
        return Handshake();
    });
    serialConnection->setDefaultBaudRate(Connection::Serial::B_115200);
    serialConnection->setDefaultPort("/dev/ttyUSB0");
    registerConnection(serialConnection);

    //----- COVER CONTROL -----
    //cover state
    CoverStateTP[0].fill("COVER_STATE", "Cover State:", "UNKNOWN");
    CoverStateTP.fill(getDeviceName(),	"COVER_STATE", "Cover", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "COVER_STATE");

    //cover positions buttons
    MoveToSP[Open].fill("Open", "Open", ISS_OFF);
    MoveToSP[Close].fill("Close", "Close", ISS_OFF);
    MoveToSP[Halt].fill("Halt", "Halt", ISS_OFF);
    MoveToSP.fill(getDeviceName(), "MOVE_TO", "Cover", MAIN_CONTROL_TAB, IP_WO, ISR_ATMOST1, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "MOVE_TO");

    //----- CALIBRATOR CONTROL -----
    //calibrator state
    CalibratorStateTP[0].fill("CALIBRATOR_STATE", "Light State:", "UNKNOWN");
    CalibratorStateTP.fill(getDeviceName(),	"CALIBRATOR_STATE", "Light Panel", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "CALIBRATOR_STATE");

    //turn calibrator on/off buttons
    TurnLightSP[Light_On].fill("On", "On", ISS_OFF);
    TurnLightSP[Light_Off].fill("Off", "Off", ISS_ON);
    TurnLightSP.fill(getDeviceName(), "TURN_LIGHT", "Light Panel", MAIN_CONTROL_TAB, IP_WO, ISR_1OFMANY, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "TURN_LIGHT");

    //maximum brightness
    MaxBrightnessNP[0].fill("MAX_BRIGHTNESS", "Max Brightness Value:", "%0.f", 0, 0, 0, 0);
    MaxBrightnessNP.fill(getDeviceName(), "MAX_BRIGHTNESS", "Light Panel", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "MAX_BRIGHTNESS");

    //current brightness value
    CurrentBrightnessNP[0].fill("CURRENT_BRIGHTNESS", "Current Brightness Value:", "%0.f", 0, 0, 0, 0);
    CurrentBrightnessNP.fill(getDeviceName(), "CURRENT_BRIGHTNESS", "Light Panel", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "CURRENT_BRIGHTNESS");

    //goto brightness
    //set values in the Handshake()
    GoToValueNP.fill(getDeviceName(), "GOTOBRIGHTNESS", "Light Panel", MAIN_CONTROL_TAB, IP_WO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "GOTOBRIGHTNESS");

    //change light value by 1 or specific value
    AdjustValueSP[Decrease].fill("Decrease", "-", ISS_OFF);
    AdjustValueSP[Increase].fill("Increase", "+", ISS_OFF);
    AdjustValueSP.fill(getDeviceName(), "ADJUST_VALUE", "Adjust Light", MAIN_CONTROL_TAB, IP_WO, ISR_ATMOST1, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "ADJUST_VALUE");

    //go to broadband/narrowband buttons
    GoToSavedSP[Broadband].fill("Broadband", "Broadband", ISS_OFF);
    GoToSavedSP[Narrowband].fill("Narrowband", "Narrowband", ISS_OFF);
    GoToSavedSP.fill(getDeviceName(), "GOTO_SAVED", "Go To Light", MAIN_CONTROL_TAB, IP_WO, ISR_ATMOST1, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "GOTO_SAVED");

    //save broadband/narrowband buttons
    SetToSavedSP[Set_Broadband].fill("Set_Broadband", "Save as Broadband", ISS_OFF);
    SetToSavedSP[Set_Narrowband].fill("Set_Narrowband", "Save as Narrowband", ISS_OFF);
    SetToSavedSP.fill(getDeviceName(), "SETTO_SAVED", "Save Value", MAIN_CONTROL_TAB, IP_WO, ISR_ATMOST1, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "SETTO_SAVED");

    //----- HEATER CONTROLS -----
    //heater state
    HeaterStateTP[0].fill("HEATER_STATE", "Heater State:", "UNKNOWN");
    HeaterStateTP.fill(getDeviceName(),	"HEATER_STATE", "Heater", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "HEATER_STATE");

    //turn heater on/off/auto/set buttons
    TurnHeaterSP[Heat_On].fill("On", "On", ISS_OFF);
    TurnHeaterSP[Heat_Off].fill("Off", "Off", ISS_ON);
    TurnHeaterSP[Heat_Auto].fill("Auto", "Auto", ISS_OFF);
    TurnHeaterSP[Heat_At_Close].fill("Heat at Close", "Heat on Close", ISS_OFF);
    TurnHeaterSP.fill(getDeviceName(), "TURN_HEATER", "Heater", MAIN_CONTROL_TAB, IP_WO, ISR_1OFMANY, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "TURN_HEATER");

    //----- INITIAL CONTROLS -----
    //stabilize light time
    //set default time
    double stabileTime = {2000};
    //load value from config file if present
    IUGetConfigNumber(getDeviceName(), "STABILIZE_TIME", "STABILIZE_TIME", &stabileTime);
    //initialize
    StabilizeTimeNP[0].fill("STABILIZE_TIME", "Stabilize Time (ms): ", "%0.f", 2000, 10000, 1000, stabileTime);
    StabilizeTimeNP.fill(getDeviceName(), "STABILIZE_TIME", "Light Panel", OPTIONS_TAB, IP_WO, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "STABILIZE_TIME");

    //autoOn
    //set default state
    ISState autoOnState = {ISS_OFF};
    //load value from config file if present
    IUGetConfigSwitch(getDeviceName(), "AUTO_ON", "AUTO_ON", &autoOnState);
    //initialize
    AutoOnSP[0].fill("AUTO_ON", "Auto ON at CLOSE", autoOnState);
    AutoOnSP.fill(getDeviceName(), "AUTO_ON", "Light Panel", OPTIONS_TAB, IP_WO, ISR_NOFMANY, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "AUTO_ON");

    //disable light
    //set default state
    ISState disableLightState = {ISS_ON};
    //load value from config file if present
    IUGetConfigSwitch(getDeviceName(), "DISABLE_LIGHT", "DISABLE_LIGHT", &disableLightState);
    //initialize
    DisableLightSP[0].fill("DISABLE_LIGHT", "Disable Light while OPEN", disableLightState);
    DisableLightSP.fill(getDeviceName(), "DISABLE_LIGHT", "Light Panel", OPTIONS_TAB, IP_WO, ISR_NOFMANY, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "DISABLE_LIGHT");

    //autoHeatOn
    //set default state
    ISState autoHeatOnState = {ISS_OFF};
    //load value from config file if present
    IUGetConfigSwitch(getDeviceName(), "AUTO_HEAT_ON", "AUTO_HEAT_ON", &autoHeatOnState);
    //initialize
    AutoHeatOnSP[0].fill("AUTO_HEAT_ON", "Auto Heat (always ON)", autoHeatOnState);
    AutoHeatOnSP.fill(getDeviceName(), "AUTO_HEAT_ON", "Heater", OPTIONS_TAB, IP_WO, ISR_NOFMANY, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "AUTO_HEAT_ON");

    //heatOnClose
    //set default state
    ISState heatOnCloseState = {ISS_OFF};
    //load value from config file if present
    IUGetConfigSwitch(getDeviceName(), "HEAT_ON_CLOSE", "HEAT_ON_CLOSE", &heatOnCloseState);
    //initialize
    HeatOnCloseSP[0].fill("HEAT_ON_CLOSE", "Heat ON at CLOSE", heatOnCloseState);
    HeatOnCloseSP.fill(getDeviceName(), "HEAT_ON_CLOSE", "Heater", OPTIONS_TAB, IP_WO, ISR_NOFMANY, 60, IPS_IDLE);
    IDSnoopDevice(getDeviceName(), "HEAT_ON_CLOSE");

    MoveToSP.onUpdate([this]
    {
        if (isConnected())
        {
            std::string coverStateText = CoverStateTP[0].getText();
            char MoveToResponse[8] = {0};
            switch (MoveToSP.findOnSwitchIndex())
            {
                case Open:
                    if(coverStateText != "Open" && coverStateText != "Moving")
                    {
                        LOG_INFO("Opening Cover");
                        if (sendCommand("O", MoveToResponse))
                        {
                            LOGF_DEBUG("OpenCover response: %s", MoveToResponse);
                            coverIsMoving = true;

                            std::string calibratorStateText = CalibratorStateTP[0].getText();
                            if (calibratorStateText != "Not Present" && calibratorStateText != "Off")
                            {
                                getCalibratorState();
                                getBrightness();
                            }
                        }
                        else
                        {
                            LOG_WARN("Open command failed");
                        }
                    }
                    break;
                case Close:
                    if(coverStateText != "Closed" && coverStateText != "Moving")
                    {
                        LOG_INFO("Closing Cover");
                        if (sendCommand("C", MoveToResponse))
                        {
                            LOGF_DEBUG("CloseCover response: %s", MoveToResponse);
                            coverIsMoving = true;

                            if (autoOn)
                            {
                                lightIsReady = false;
                            }
                        }
                        else
                        {
                            LOG_WARN("Close command failed");
                        }
                    }
                    break;
                case Halt:
                    if(coverStateText == "Moving")
                    {
                        LOG_INFO("Halting Cover");
                        if (sendCommand("H", MoveToResponse))
                        {
                            LOGF_DEBUG("HaltCover response: %s", MoveToResponse);
                            coverIsMoving = true;
                        }
                        else
                        {
                            LOG_WARN("Halt command failed");
                        }
                    }
                    break;
            }

            //reset switch
            MoveToSP.reset();
            //set property state back to idle
            MoveToSP.setState(IPS_IDLE);
            //inform INDI of the operation
            MoveToSP.apply();
        }
        else
        {
            LOG_WARN("Must connect first");
        }
    });//end of MoveToSP

    //CalibratorOn/Off
    TurnLightSP.onUpdate([this]
    {
        if (isConnected())
        {
            char TurnLightResponse[8] = {0};
            std::string calibratorStateText = CalibratorStateTP[0].getText();
            std::string coverStateText = CoverStateTP[0].getText();
            switch (TurnLightSP.findOnSwitchIndex())
            {
                case Light_On:
                    //if DisabledLight is not set to true
                    if (!lightDisabled)
                    {
                        //if light is not already on
                        if (calibratorStateText == "Off")
                        {
                            LOG_INFO("Turning Light ON");
                            setBrightness(0);
                        }
                    }
                    else if (lightDisabled && coverStateText == "Closed")
                    {
                        //if light is not already on
                        if (calibratorStateText == "Off")
                        {
                            LOG_INFO("Turning Light ON");
                            setBrightness(0);
                        }
                    }
                    else
                    {
                        LOG_WARN("Light is set to disabled while cover is OPEN");
                        TurnLightSP[Light_On].setState(ISS_OFF);
                        TurnLightSP[Light_Off].setState(ISS_ON);
                    }
                    break;
                case Light_Off:
                    //if light is not already off
                    if (calibratorStateText != "Off")
                    {
                        LOG_INFO("Turning Light OFF");
                        //if light already off ignore
                        if (sendCommand("F", TurnLightResponse))
                        {
                            LOGF_DEBUG("CalibratorOff response: %s", TurnLightResponse);

                            //set CalibratorState to Off (1)
                            CalibratorStateTP[0].setText("Off");
                            CalibratorStateTP.apply();

                            //set CurrentBrightness to Off (0)
                            CurrentBrightnessNP[0].setValue(0);
                            CurrentBrightnessNP.apply();
                        }
                        else
                        {
                            LOG_WARN("Turn light OFF command failed");
                        }
                        break;
                    }
            }
            //set property state back to idle
            TurnLightSP.setState(IPS_IDLE);
            //inform INDI of the operation
            TurnLightSP.apply();
        }
        else
        {
            LOG_WARN("Must connect first");
        }
    });//end of TurnLightSP

    //change light to specific value
    GoToValueNP.onUpdate([this]
    {
        if (isConnected())
        {
            //if light is not disable unless closed then activate light
            if (!lightDisabled)
            {
                LOGF_DEBUG("Light is not disabled. Setting brightness to %d", static_cast<int>(GoToValueNP[0].getValue()));
                LOGF_INFO("Setting brightness to %d", static_cast<int>(GoToValueNP[0].getValue()));
                setBrightness(GoToValueNP[0].getValue());

                TurnLightSP[Light_On].setState(ISS_ON);
                TurnLightSP[Light_Off].setState(ISS_OFF);
                
                //set property state back to idle
                GoToValueNP.setState(IPS_IDLE);
            }
            //check that cover is closed before activating light
            else
            {
                if (CoverStateTP[0].getText() == std::string("Closed"))
                {
                    LOGF_DEBUG("Light disabled but cover is CLOSED. Setting brightness to %d", static_cast<int>(GoToValueNP[0].getValue()));
                    LOGF_INFO("Setting brightness to %d", static_cast<int>(GoToValueNP[0].getValue()));
                    setBrightness(GoToValueNP[0].getValue());

                    TurnLightSP[Light_On].setState(ISS_ON);
                    TurnLightSP[Light_Off].setState(ISS_OFF);

                    //set property state back to idle
                    GoToValueNP.setState(IPS_IDLE);
                }
                else
                {
                    LOG_WARN("Light disabled while cover is OPEN");
                }
            }
             //inform INDI of the operation
            TurnLightSP.apply();
            GoToValueNP.apply();
        }
        else
        {
            LOG_WARN("Must connect first");
        }
    });//end of GoToValueNP

    //incremental change brightness
    AdjustValueSP.onUpdate([this]
    {
        if (TurnLightSP.findOnSwitchIndex() == Light_On)
        {
            switch (AdjustValueSP.findOnSwitchIndex())
            {
                case Decrease:
                    if (CurrentBrightnessNP[0].getValue() - 1 >= 1)
                    {
                        LOG_INFO("Decreasing Brightness");
                        double brightness = CurrentBrightnessNP[0].getValue() - 1;
                        setBrightness(brightness);
                    }
                    else
                    {
                        LOG_ERROR("Brightness cannot go below 1");
                    }
                    break;
                case Increase:
                    if (CurrentBrightnessNP[0].getValue() + 1 <= MaxBrightnessNP[0].getValue())
                    {
                        LOG_INFO("Increasing Brightness");
                        int brightness = CurrentBrightnessNP[0].getValue() + 1;
                        setBrightness(brightness);
                    }
                    else
                    {
                        LOG_ERROR("Cannot go above Max Brightness");
                    }
                    break;
            }
        }
        else
        {
            LOG_WARN("Must turn Light ON");
        }

        //reset switch
        AdjustValueSP.reset();
        //set property state back to idle
        AdjustValueSP.setState(IPS_IDLE);
        //inform INDI of the operation
        AdjustValueSP.apply();
    });//end of AdjustValueSP

    //Go to preset BB / NB values
    GoToSavedSP.onUpdate([this]
    {
        char GoToSavedResponse[8] = {0};
        if (TurnLightSP.findOnSwitchIndex() == Light_On)
        {
            switch (GoToSavedSP.findOnSwitchIndex())
            {
                case Broadband:
                    LOG_INFO("Setting Brightness to Broadband value");
                    //get broadband value

                    if (sendCommand("GB", GoToSavedResponse))
                    {
                        LOGF_DEBUG("GoTo BB response: %s", GoToSavedResponse);
                        //convert response to a double
                        setBrightness(std::stod(GoToSavedResponse));
                    }
                    else
                    {
                        LOG_WARN("GoTo Broadband command failed");
                    }
                    break;
                case Narrowband:
                    LOG_INFO("Setting Brightness to Narrowband value");
                    //get narrowband value
                    if (sendCommand("GN", GoToSavedResponse))
                    {
                        LOGF_DEBUG("GoTo NB response: %s", GoToSavedResponse);
                        //convert response to a double
                        setBrightness(std::stod(GoToSavedResponse));
                    }
                    else
                    {
                        LOG_WARN("GoTo Narrowband command failed");
                    }
                    break;
            }
        }
        else
        {
            LOG_WARN("Must turn light on to go to preset value");
        }

        //reset switch
        GoToSavedSP.reset();
        //set property state back to idle
        GoToSavedSP.setState(IPS_IDLE);
        //inform INDI of the operation
        GoToSavedSP.apply();
    });//end of GoToSavedSP

    //Save preset BB / NB values
    SetToSavedSP.onUpdate([this]
    {
        char SetToSavedResponse[8] = {0};
        if (TurnLightSP.findOnSwitchIndex() == Light_On)
        {
            switch (SetToSavedSP.findOnSwitchIndex())
            {
                case Set_Broadband:
                    LOG_INFO("Saving Broadband Brightness");

                    if (sendCommand("DB", SetToSavedResponse))
                    {
                        LOGF_DEBUG("Set BB response: %s", SetToSavedResponse);
                    }
                    else
                    {
                        LOG_WARN("Save Broadband value command failed");
                    }
                    break;
                case Set_Narrowband:
                    LOG_INFO("Saving Narrowband Brightness");
                    if (sendCommand("DN", SetToSavedResponse))
                    {
                        LOGF_DEBUG("Set NB response: %s", SetToSavedResponse);
                    }
                    else
                    {
                        LOG_WARN("Save Narrowband value command failed");
                    }
                    break;
            }
        }
        else
        {
            LOG_WARN("Must turn light on to save");
        }

        //reset switch
        SetToSavedSP.reset();
        //set property state back to idle
        SetToSavedSP.setState(IPS_IDLE);
        //inform INDI of the operation
        SetToSavedSP.apply();
    });//end of SetToSavedSP

    //StabilizeTime
    StabilizeTimeNP.onUpdate([this]
    {
        if (isConnected())
        {
            setStabilizeTime();
        }
        else
        {
            LOG_WARN("Not connected, StabilizeTime change will be automatically applied at startup");
        }

        //set property back to idle
        StabilizeTimeNP.setState(IPS_IDLE);
        //inform INDI of the operation
        StabilizeTimeNP.apply();

        saveConfig();
    });//end of StabilizeTimeNP

    //AutoOn
    AutoOnSP.onUpdate([this]
    {
        if (isConnected())
        {
            setAutoOn();
        }
        else
        {
            LOG_WARN("Not connected, AutoOn change will be automatically applied at startup");
        }

        //set property back to idle
        AutoOnSP.setState(IPS_IDLE);
        //inform INDI of the operation
        AutoOnSP.apply();

        saveConfig();
    });//end of AutoOnSP

    DisableLightSP.onUpdate([this]
    {
        if (isConnected())
        {
            setLightDisabled();

            //set property back to idle
            DisableLightSP.setState(IPS_IDLE);
            //inform INDI of the operation
            DisableLightSP.apply();
        }

        saveConfig();
    });//end of DisableLightSP

    //Heater On/Off/Auto/HeatOnClose
    TurnHeaterSP.onUpdate([this]
    {
        if (isConnected())
        {
            char HeaterResponse[8] = {0};
            std::string heaterStateText = HeaterStateTP[0].getText();
            switch (TurnHeaterSP.findOnSwitchIndex())
            {
                case Heat_On:
                    if (heaterStateText != "On" && heaterStateText != "Error")
                    {
                        LOG_INFO("Turning heater ON");
                        if (sendCommand("W", HeaterResponse))
                        {
                            LOGF_DEBUG("Heater response: %s", HeaterResponse);
                        }
                        else
                        {
                            LOG_WARN("Set Heater ON command failed");
                        }
                    }
                    break;
                case Heat_Off:
                    if (heaterStateText != "Off")
                    {
                        LOG_INFO("Turning heater OFF");
                        if (sendCommand("w", HeaterResponse))
                        {
                            LOGF_DEBUG("Heater response: %s", HeaterResponse);
                        }
                        else
                        {
                            LOG_WARN("Set Heater OFF command failed");
                        }
                    }
                    break;
                case Heat_Auto:
                    if (!autoHeatOn)
                    {
                        LOG_INFO("Setting heater to AUTO");
                        if (sendCommand("Q", HeaterResponse))
                        {
                            LOGF_DEBUG("Heater response: %s", HeaterResponse);
                            autoHeatOn = true;
                            heatOnClose = false;
                        }
                        else
                        {
                            LOG_WARN("Enable Heater AUTO command failed");
                        }
                    }
                    else
                    {
                        LOG_INFO("Turning OFF auto heating");
                        if (sendCommand("q", HeaterResponse))
                        {
                            LOGF_DEBUG("Heater response: %s", HeaterResponse);
                            autoHeatOn = false;
                        }
                        else
                        {
                            LOG_WARN("Disable Heater AUTO command failed");
                        }
                    }
                    break;
                case Heat_At_Close:
                    if (!heatOnClose)
                    {
                        LOG_INFO("Setting heater to turn ON at CLOSE");
                        if (sendCommand("E", HeaterResponse))
                        {
                            LOGF_DEBUG("Heater response: %s", HeaterResponse);
                            heatOnClose = true;
                            autoHeatOn = false;
                        }
                        else
                        {
                            LOG_WARN("Enable Heat On Close command failed");
                        }
                    }
                    else
                    {
                        LOG_INFO("Turning heat on close OFF");
                        if (sendCommand("e", HeaterResponse))
                        {
                            LOGF_DEBUG("Heater response: %s", HeaterResponse);
                            heatOnClose = false;
                        }
                        else
                        {
                            LOG_WARN("Disable Heat On Close command failed");
                        }
                    }
                    break;
            }
            getHeaterState();
        }
        else
        {
            LOG_WARN("Must connect first");
        }
    });//end of TurnHeaterSP

    //AutoHeatOn
    AutoHeatOnSP.onUpdate([this]
    {
        if (isConnected())
        {
            setAutoHeatOn();
            heatModeIsChanging = true;
        }
        else
        {
            LOG_WARN("Not connected, AutoHeatOn change will be automatically applied at startup");
        }

        //set property back to idle
        AutoHeatOnSP.setState(IPS_IDLE);
        //inform INDI of the operation
        AutoHeatOnSP.apply();

        saveConfig();
    });//end of AutoHeatOnSP

    //HeatOnClose
    HeatOnCloseSP.onUpdate([this]
    {
        if (isConnected())
        {
            setHeatOnClose();
            heatModeIsChanging = true;
        }
        else
        {
            LOG_WARN("Not connected, HeatOnClose change will be automatically applied at startup");
        }

        //set property back to idle
        HeatOnCloseSP.setState(IPS_IDLE);
        //inform INDI of the operation
        HeatOnCloseSP.apply();

        saveConfig();
    });//end of HeatOnCloseSP

    //add controls to the driver
    addPollPeriodControl();
    addConfigurationControl();
    addDebugControl();

    //set driver interface
    setDriverInterface(getDriverInterface() | AUX_INTERFACE);

    return true;
}//end of initProperties

bool DarkLight_CoverCalibrator::Handshake()
{
    //get port
    PortFD = serialConnection->getPortFD();

    //verify connected
    if (PortFD == -1)
    {
        LOG_ERROR("Serial port is not open or invalid.");
        return false;
    }
    else
    {
        LOG_DEBUG("Serial port is open");
    }

    // Send handshake command 'Z' and expect '?' in response
    const char *handshakeCommand = "Z";
    char response[8] = {0}; // Assuming 8 bytes is sufficient for the response

    LOG_DEBUG("Sending handshake command");

    if (!sendCommand(handshakeCommand, response))
    {
        LOG_ERROR("Failed to send handshake command. Check baud rate");
        return false;
    }

    if (response[0] != '?')
    {
        LOGF_ERROR("Invalid handshake response. Expected '?', but received: %s", response);
        return false;
    }

    return true;
}//end of Handshake

bool DarkLight_CoverCalibrator::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        //define cover properties if present
        getCoverState();
        if (CoverStateTP[0].getText() != std::string("Not Present"))
        {
            defineProperty(CoverStateTP);
            defineProperty(MoveToSP);
        }
        else
        {
            LOG_INFO("Cover is reported as Not Present");
        }
        
        //define calibrator properties if present
        getCalibratorState();
        std::string calibratorStateText = CalibratorStateTP[0].getText();
        if (calibratorStateText != "Not Present")
        {
            //StabilizeTime
            setStabilizeTime();

            //AutoON
            setAutoOn();

            //lightDisable
            setLightDisabled();

            //get MaxBrightness
            LOG_DEBUG("Getting Max Brightness");
            char MaxBrightnessResponse[8] = {0};
            if (!sendCommand("M", MaxBrightnessResponse))
            {
            }
            else
            {
                LOGF_DEBUG("MaxBrightness response: %s", MaxBrightnessResponse);
                MaxBrightnessNP[0].setValue(std::stoi(MaxBrightnessResponse));
                MaxBrightnessNP.apply();

                //set GoToBrightness max value
                GoToValueNP[0].fill("GOTOBRIGHTNESS", "Go To Brightness Value:", "%0.f", 1, MaxBrightnessNP[0].getValue(), 1,
                                    MaxBrightnessNP[0].getValue());
            }

            //if light is on change switch
            if (calibratorStateText != "Off")
            {
                TurnLightSP[Light_On].setState(ISS_ON);
                TurnLightSP[Light_Off].setState(ISS_OFF);
                TurnLightSP.apply();

                getBrightness();
            }

            defineProperty(CalibratorStateTP);
            defineProperty(TurnLightSP);
            defineProperty(MaxBrightnessNP);
            defineProperty(CurrentBrightnessNP);
            defineProperty(GoToValueNP);
            defineProperty(AdjustValueSP);
            defineProperty(GoToSavedSP);
            defineProperty(SetToSavedSP);
            defineProperty(StabilizeTimeNP);
            defineProperty(AutoOnSP);
            defineProperty(DisableLightSP);
        }
        else
        {
            LOG_DEBUG("Light panel is reported as Not Present");
        }

        //define heater properties if present
        getHeaterState();
        if (HeaterStateTP[0].getText() != std::string("Not Present"))
        {
            defineProperty(AutoHeatOnSP);
            defineProperty(HeatOnCloseSP);
            defineProperty(HeaterStateTP);
            defineProperty(TurnHeaterSP);
        }
        else
        {
            LOG_INFO("Heater is reported as Not Present");
        }

        SetTimer(getCurrentPollingPeriod());
    }
    else
    {
        deleteProperty(CoverStateTP);
        deleteProperty(MoveToSP);
        deleteProperty(CalibratorStateTP);
        deleteProperty(TurnLightSP);
        deleteProperty(MaxBrightnessNP);
        deleteProperty(CurrentBrightnessNP);
        deleteProperty(GoToValueNP);
        deleteProperty(AdjustValueSP);
        deleteProperty(GoToSavedSP);
        deleteProperty(SetToSavedSP);
        deleteProperty(StabilizeTimeNP);
        deleteProperty(AutoOnSP);
        deleteProperty(DisableLightSP);
        deleteProperty(AutoHeatOnSP);
        deleteProperty(HeatOnCloseSP);
        deleteProperty(HeaterStateTP);
        deleteProperty(TurnHeaterSP);
    }

    return true;
}//end of updateProperties

bool DarkLight_CoverCalibrator::sendCommand(const char *command, const char *response)
{
    std::lock_guard<std::mutex> lock(serialMutex); //acquire mutex for thread safety

    if (PortFD == -1)
    {
        return false; //cannot send if port is not open
    }

    int nbytes_read = 0, nbytes_written = 0, tty_rc = 0;
    char res[8] = {0};

    //retry a maximum of 3 times
    const int maxRetries = 3;
    int retryCount = 0;

    //form the command
    std::string commandToSend = "<";
    commandToSend += command;
    commandToSend += ">";

    LOGF_DEBUG("Sending command: %s", commandToSend.c_str());

    do
    {
        //set a timeout of 5 seconds
        int timeoutMs = 5000;

        //use a loop with select to monitor the serial port with a timeout
        while (true)
        {
            tcflush(PortFD, TCIOFLUSH);
            if ((tty_rc = tty_write_string(PortFD, commandToSend.c_str(), &nbytes_written)) != TTY_OK)
            {
                char errorMessage[MAXRBUF];
                tty_error_msg(tty_rc, errorMessage, MAXRBUF);
                LOGF_ERROR("Serial write error: %s", errorMessage);
                return false;
            }

            struct timeval timeout;
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(PortFD, &readfds);

            int selectResult = select(PortFD + 1, &readfds, nullptr, nullptr, &timeout);
            if (selectResult == -1)
            {
                LOGF_ERROR("Serial select error: %s", strerror(errno));
                return false;
            }
            else if (selectResult == 0)
            {
                LOG_ERROR("Serial read timed out");
                break; //exit the inner loop and try again (retry)
            }
            else
            {
                //data is available for reading, proceed with tty_read_section
                if ((tty_rc = tty_read_section(PortFD, res, '>', 1, &nbytes_read)) == TTY_OK)
                {
                    //response received successfully
                    LOGF_DEBUG("Response received: %s", res);
                    res[nbytes_read - 1] = '\0';

                    //ensure response is copied back to the caller's buffer
                    strcpy(const_cast<char*>(response), res + 1);
                    return true; //success
                }
                else
                {
                    LOGF_ERROR("Serial read error: %s", res);
                }
            }
        }

        //increment the retry count
        retryCount++;
    }
    while (retryCount < maxRetries);

    LOG_ERROR("Maximum retry attempts reached. Transmission failed.");
    return false; // Error
}//end of sendCommand

bool DarkLight_CoverCalibrator::mainValues()
{
    //get CoverState
    const std::string& coverState = CoverStateTP[0].getText();
    if (coverState != "Not Present" && coverIsMoving)
    {
        getCoverState();
    }

    //get CalibratorState
    const std::string& calibratorState = CalibratorStateTP[0].getText();
    if (coverState != "Not Present" && !lightIsReady)
    {
        getCalibratorState();

        //check brightness if light on
        if (CalibratorStateTP[0].getText() != std::string("Ready"))
        {
            getBrightness();

            //change switch state visual
            TurnLightSP[Light_On].setState(ISS_ON);
            TurnLightSP[Light_Off].setState(ISS_OFF);
            TurnLightSP.apply();
        }//end of Brightness
    }

    //refresh HeaterState if On/Auto/Heat On Close is set
    const std::string& heaterStateTP = HeaterStateTP[0].getText();
    const int turnHeaterSP = TurnHeaterSP.findOnSwitchIndex();
    if ((heaterStateTP != "Not Present" && turnHeaterSP != 1) || (heatModeIsChanging))
    {
        getHeaterState();
    }

    return true;
}//end of mainValues

void DarkLight_CoverCalibrator::TimerHit()
{
    if (!isConnected())
    {
        return;
    }

    mainValues();
    SetTimer(getCurrentPollingPeriod());
}//end of TimerHit

void DarkLight_CoverCalibrator::setStabilizeTime()
{
    LOG_DEBUG("Setting StabilizeTime");
    //compose command string
    double value = StabilizeTimeNP[0].getValue();
    int intValue = static_cast<int>(value);
    std::string command = "S";
    command += std::to_string(intValue);  //append integer value

    //send command
    char StabilizeTimeResponse[8] = {0};
    if (sendCommand(command.c_str(), StabilizeTimeResponse))
    {
        LOGF_DEBUG("StabilizeTime response: %s", StabilizeTimeResponse);
    }
    else
    {
        LOG_WARN("StabilizeTime command failed");
    }
}//end of setStabilizeTime

void DarkLight_CoverCalibrator::setAutoOn()
{
    LOG_DEBUG("Setting autoOn");
    char AutoOnResponse[8] = {0};
    switch (AutoOnSP.findOnSwitchIndex())
    {
        case Light_AutoOn:
        {
            LOG_DEBUG("Setting AutoOn TRUE");

            if (sendCommand("A", AutoOnResponse))
            {
                autoOn = true;
            }
            else
            {
                LOG_WARN("Enable AutoOn command failed");
            }
            break;
        }

        default:
        {
            LOG_DEBUG("Setting AutoOn FALSE");
            if (sendCommand("a", AutoOnResponse))
            {
                autoOn = false;
            }
            else
            {
                LOG_WARN("Disable AutoOn command failed");
            }
            break;
        }
    }//end of switch

    LOGF_DEBUG("AutoOn response: %s", AutoOnResponse);
}//end of setAutoOn

void DarkLight_CoverCalibrator::setLightDisabled()
{
    LOG_DEBUG("Setting lightDisabled");
    switch (DisableLightSP.findOnSwitchIndex())
    {
        case Light_Disable:
            LOG_DEBUG("Setting DisableLight TRUE");
            lightDisabled = true;
            break;
        default:
            LOG_DEBUG("Setting DisableLight FALSE");
            lightDisabled = false;
            break;
    }
}//end of setLightDisabled

void DarkLight_CoverCalibrator::getCoverState()
{
    char CoverStateResponse[8] = {0};
    LOG_DEBUG("Get CoverState");
    if (!sendCommand("P", CoverStateResponse))
    {
        LOG_ERROR("CoverState ERROR");
    }
    else
    {
        LOGF_DEBUG("CoverState response: %s", CoverStateResponse);

        //handle potential multi-character responses
        if (strlen(CoverStateResponse) > 1)
        {
            LOG_WARN("CoverState: Unexpected multi-character response");
            CoverStateTP[0].setText("Invalid Response");
        }
        else
        {
            //process the response
            int responseValue = CoverStateResponse[0] - '0';
            switch (responseValue)
            {
                case 0:
                    CoverStateTP[0].setText("Not Present");
                    break;
                case 1:
                    CoverStateTP[0].setText("Closed");
                    coverIsMoving = false;
                    LOG_INFO("Cover is CLOSED");
                    if (autoOn)
                    {
                        LOG_INFO("Activating light");
                    }
                    break;
                case 2:
                    CoverStateTP[0].setText("Moving");
                    break;
                case 3:
                    CoverStateTP[0].setText("Open");
                    coverIsMoving = false;
                    LOG_INFO("Cover is OPEN");
                    break;
                case 4:
                    CoverStateTP[0].setText("Unknown");
                    coverIsMoving = false;
                    LOG_WARN("Cover in UNKNOWN state");
                    break;
                case 5:
                    CoverStateTP[0].setText("Error");
                    coverIsMoving = false;
                    LOG_ERROR("Cover reported ERROR");
                    break;
                default:
                    LOG_WARN("CoverState: Invalid response value");
                    CoverStateTP[0].setText("Invalid Response");
            }
            CoverStateTP.setState(IPS_IDLE);
            CoverStateTP.apply();
        }
    }
}//end of getCoverState

void DarkLight_CoverCalibrator::getCalibratorState()
{
    char GetCalibratorStateResponse[8] = {0};
    LOG_DEBUG("Get CalibratorState");
    if (!sendCommand("L", GetCalibratorStateResponse))
    {
        LOG_ERROR("CalibratorState ERROR");
    }
    else
    {
        LOGF_DEBUG("CalibratorState response: %s", GetCalibratorStateResponse);

        //handle potential multi-character responses
        if (strlen(GetCalibratorStateResponse) > 1)
        {
            LOG_WARN("CalibratorState: Unexpected multi-character response");
            CalibratorStateTP[0].setText("Invalid Response");
        }
        else
        {
            int responseValue = GetCalibratorStateResponse[0] - '0';
            switch (responseValue)
            {
                case 0:
                    CalibratorStateTP[0].setText("Not Present");
                    break;
                case 1:
                    CalibratorStateTP[0].setText("Off");
                    break;
                case 2:
                    CalibratorStateTP[0].setText("Not Ready");
                    break;
                case 3:
                    CalibratorStateTP[0].setText("Ready");
                    lightIsReady = true;
                    break;
                case 4:
                    CalibratorStateTP[0].setText("Unknown");
                    break;
                case 5:
                    CalibratorStateTP[0].setText("Error");
                    break;
                default:
                    LOG_WARN("CalibratorState: Invalid response value");
                    CalibratorStateTP[0].setText("Invalid Response");
            }

            if (responseValue != 0 && responseValue != 1)
            {
                //set light button to ON
                TurnLightSP[Light_On].setState(ISS_ON);
                TurnLightSP[Light_Off].setState(ISS_OFF);
            }
            else
            {
                //set light button to OFF
                TurnLightSP[Light_On].setState(ISS_OFF);
                TurnLightSP[Light_Off].setState(ISS_ON);
            }
            TurnLightSP.apply();

            CalibratorStateTP.setState(IPS_IDLE);
            CalibratorStateTP.apply();
        }
    }
}//end of getCalibratorState

void DarkLight_CoverCalibrator::getBrightness()
{
    char BrightnessResponse[8] = {0};
    LOG_DEBUG("Getting Brightness");
    //get brightness response
    if (!sendCommand("B", BrightnessResponse))
    {
    }
    else
    {
        LOGF_DEBUG("CurrentBrightness response: %s", BrightnessResponse);

        //handle potential multi-character responses
        if (strlen(BrightnessResponse) > 3)
        {
        }
        else
        {
            int brightnessValue = std::stoi(BrightnessResponse);

            //check range
            if (brightnessValue >= 0 && brightnessValue <= MaxBrightnessNP[0].getValue())
            {
                CurrentBrightnessNP[0].setValue(brightnessValue);
                CurrentBrightnessNP.setState(IPS_IDLE);
                CurrentBrightnessNP.apply();
            }
            else
            {
                LOG_WARN("Brightness value out of range");
            }
        }
    }
}//end of getBrightness

void DarkLight_CoverCalibrator::setBrightness(double BrightnessValue)
{
    //convert double to int
    if (BrightnessValue == 0)
    {
        BrightnessValue = MaxBrightnessNP[0].getValue();
    }
    int intValue = static_cast<int>(BrightnessValue);
    std::string command = "T";
    command += std::to_string(intValue);  //append value

    //send command
    char response[8] = {0};
    LOG_DEBUG("Setting Brightness");
    if (!sendCommand(command.c_str(), response))
    {
    }
    else
    {
        LOGF_DEBUG("SetBrightness response: %s", response);
        lightIsReady = false;
    }
}//end of setBrightness

void DarkLight_CoverCalibrator::setAutoHeatOn()
{
    LOG_DEBUG("Setting autoHeatOn");
    char AutoHeatOnResponse[8] = {0};
    switch (AutoHeatOnSP.findOnSwitchIndex())
    {
        case Heat_AutoOn:
        {
            LOG_DEBUG("Setting AutoHeatOn TRUE");
            if (heatOnClose)
            {
                LOG_WARN("Heat On Close currently enabled. Switching modes.");
                heatOnClose = false;
                HeatOnCloseSP[0].setState(ISS_OFF);
                HeatOnCloseSP.setState(IPS_IDLE);
                HeatOnCloseSP.apply();
            }

            if (sendCommand("Q", AutoHeatOnResponse))
            {
                autoHeatOn = true;
                LOG_INFO("Auto control of heating enabled");
                
            }
            else
            {
                LOG_WARN("Enable AutoHeatOn command failed");
            }
            break;
        }

        default:
        {
            LOG_DEBUG("Setting AutoHeatOn FALSE");
            if (sendCommand("q", AutoHeatOnResponse))
            {
                autoHeatOn = false;
                LOG_DEBUG("Auto control of heating disabled");
            }
            else
            {
                LOG_WARN("Disable AutoHeatOn command failed");
            }
            break;
        }
    }//end of switch

    LOGF_DEBUG("AutoHeatOn response: %s", AutoHeatOnResponse);
}//end of setAutoOn

void DarkLight_CoverCalibrator::setHeatOnClose()
{
    LOG_DEBUG("Setting HeatOnClose");
    char HeatOnCloseResponse[8] = {0};
    switch (HeatOnCloseSP.findOnSwitchIndex())
    {
        case Heat_OnClose:
        {
            LOG_DEBUG("Setting HeatOnClose TRUE");
            if (autoHeatOn)
            {
                LOG_WARN("Auto Heat currently enabled. Switching modes.");
                autoHeatOn = false;
                AutoHeatOnSP[0].setState(ISS_OFF);
                AutoHeatOnSP.setState(IPS_IDLE);
                AutoHeatOnSP.apply();
            }

            if (sendCommand("E", HeatOnCloseResponse))
            {
                heatOnClose = true;
                LOG_INFO("Heater set to turn ON after cover closes");
                
            }
            else
            {
                LOG_WARN("Enable HeatOnClose command failed");
            }
            break;
        }

        default:
        {
            LOG_DEBUG("Setting HeatOnClose FALSE");
            if (sendCommand("e", HeatOnCloseResponse))
            {
                heatOnClose = false;
                LOG_DEBUG("Heat on close feature disabled");
            }
            else
            {
                LOG_WARN("Disable HeatOnClose command failed");
            }
            break;
        }
    }//end of switch

    LOGF_DEBUG("HeatOnClose response: %s", HeatOnCloseResponse);
}//end of setHeatOnClose

void DarkLight_CoverCalibrator::getHeaterState()
{
    char HeaterStateResponse[8] = {0};
    LOG_DEBUG("Get HeaterState");
    if (!sendCommand("R", HeaterStateResponse))
    {
        LOG_ERROR("HeaterState ERROR");
    }
    else
    {
        LOGF_DEBUG("HeaterState response: %s", HeaterStateResponse);

        //handle potential multi-character responses
        if (strlen(HeaterStateResponse) > 1)
        {
            LOG_WARN("HeaterState: Unexpected multi-character response");
            HeaterStateTP[0].setText("Invalid Response");
        }
        else
        {
            //process the response
            int responseValue = HeaterStateResponse[0] - '0';
            switch (responseValue)
            {
                case 0:
                    HeaterStateTP[0].setText("Not Present");
                    break;
                case 1:
                    HeaterStateTP[0].setText("Off");
                    TurnHeaterSP[Heat_On].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Off].setState(ISS_ON);
                    TurnHeaterSP[Heat_Auto].setState(ISS_OFF);
                    TurnHeaterSP[Heat_At_Close].setState(ISS_OFF);
                    break;
                case 2:
                    HeaterStateTP[0].setText("Auto");
                    TurnHeaterSP[Heat_On].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Off].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Auto].setState(ISS_ON);
                    TurnHeaterSP[Heat_At_Close].setState(ISS_OFF);
                    break;
                case 3:
                    HeaterStateTP[0].setText("On");
                    TurnHeaterSP[Heat_On].setState(ISS_ON);
                    TurnHeaterSP[Heat_Off].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Auto].setState(ISS_OFF);
                    TurnHeaterSP[Heat_At_Close].setState(ISS_OFF);
                    break;
                case 4:
                    HeaterStateTP[0].setText("Unknown");
                    if (autoHeatOn)
                    {
                        TurnHeaterSP[Heat_On].setState(ISS_OFF);
                        TurnHeaterSP[Heat_Auto].setState(ISS_ON);
                        TurnHeaterSP[Heat_At_Close].setState(ISS_OFF);
                    }
                    else if (heatOnClose)
                    {
                        TurnHeaterSP[Heat_On].setState(ISS_OFF);
                        TurnHeaterSP[Heat_Auto].setState(ISS_OFF);
                        TurnHeaterSP[Heat_At_Close].setState(ISS_ON);
                    }
                    else
                    {
                        TurnHeaterSP[Heat_On].setState(ISS_ON);
                        TurnHeaterSP[Heat_Auto].setState(ISS_OFF);
                        TurnHeaterSP[Heat_At_Close].setState(ISS_OFF);
                    }
                    
                    TurnHeaterSP[Heat_Off].setState(ISS_OFF);                    
                    break;
                case 5:
                    HeaterStateTP[0].setText("Error");
                    TurnHeaterSP[Heat_On].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Off].setState(ISS_ON);
                    TurnHeaterSP[Heat_Auto].setState(ISS_OFF);
                    TurnHeaterSP[Heat_At_Close].setState(ISS_OFF);
                    break;
                case 6:
                    HeaterStateTP[0].setText("Set");
                    TurnHeaterSP[Heat_On].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Off].setState(ISS_OFF);
                    TurnHeaterSP[Heat_Auto].setState(ISS_OFF);
                    TurnHeaterSP[Heat_At_Close].setState(ISS_ON);
                    break;
                default:
                    LOG_WARN("HeaterState: Invalid response value");
                    HeaterStateTP[0].setText("Invalid Response");
            }
            if (responseValue == 1)
            {
                heatModeIsChanging = false;
            }
            HeaterStateTP.apply();
            TurnHeaterSP.apply();
        }
    }
}//end of getHeaterState