/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \page ws2812fx.html
    \title WS2812FX Control
    \brief Plug-In to control WS2812FX over USB

    \ingroup plugins
    \ingroup nymea-plugins

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{ThingClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.
s
    \quotefile plugins/deviceplugins/ws2812fx/devicepluginws2812fx.json
*/
#include <QColor>
#include "integrationpluginws2812fx.h"
#include "plugininfo.h"

#include "nymealightserialinterface.h"

IntegrationPluginWs2812fx ::IntegrationPluginWs2812fx ()
{
}

void IntegrationPluginWs2812fx::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString interface = thing->paramValue(ws2812fxThingSerialPortParamTypeId).toString();

    if (m_usedInterfaces.contains(interface)) {
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("This serial port is already used."));
        return;
    }

    NymeaLightSerialInterface *lightInterface = new NymeaLightSerialInterface(interface, thing);
    NymeaLight *light = new NymeaLight(lightInterface, this);
    lightInterface->setParent(light);

    if (!lightInterface->open()) {
        qCWarning(dcWs2812fx()) << "Could not open interface" << interface;
        light->deleteLater();
        info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error opening serial port."));
        return;
    }

    connect(light, &NymeaLight::availableChanged, thing, [=](bool available){
        qCDebug(dcWs2812fx()) << thing << "available changed" << available;
        thing->setStateValue(ws2812fxConnectedStateTypeId, available);
    });

    qCDebug(dcWs2812fx()) << "Setup successfully serial port" << interface;
    thing->setStateValue(ws2812fxConnectedStateTypeId, true);
    m_usedInterfaces.append(interface);
    m_lights.insert(thing, light);
    info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginWs2812fx::discoverThings(ThingDiscoveryInfo *info)
{
    // Create the list of available serial interfaces

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {

        qCDebug(dcWs2812fx()) << "Found serial port:" << port.portName();
        QString description = port.manufacturer() + " " + port.description();
        ThingDescriptor descriptor(info->thingClassId(), port.portName(), description);
        foreach (Thing *existingThing, myThings().filterByParam(ws2812fxThingSerialPortParamTypeId, port.portName())) {
            descriptor.setThingId(existingThing->id());
        }
        ParamList parameters;
        parameters.append(Param(ws2812fxThingSerialPortParamTypeId, port.portName()));
        descriptor.setParams(parameters);
        info->addThingDescriptor(descriptor);
    }
    info->finish(Thing::ThingErrorNoError);
}


void IntegrationPluginWs2812fx::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    NymeaLight *light = m_lights.value(thing);
    if (!light || !light->available()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (action.actionTypeId() == ws2812fxColorActionTypeId) {
        QColor color = action.param(ws2812fxColorActionColorParamTypeId).value().value<QColor>();
        qCDebug(dcWs2812fx()) << "Set color to" << color.name(QColor::HexRgb);
        NymeaLightInterfaceReply *reply = light->setColor(color);
        connect(info, &ThingActionInfo::aborted, reply, &NymeaLightInterfaceReply::finished);
        connect(reply, &NymeaLightInterfaceReply::finished, this, [=](){
            if (reply->status() != NymeaLightInterface::StatusSuccess) {
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
            qCDebug(dcWs2812fx()) << "Set color finished successfully" << color.name(QColor::HexRgb);
            thing->setStateValue(ws2812fxColorStateTypeId, color);
            info->finish(Thing::ThingErrorNoError);
        });
    }

//    QByteArray command;
//    if (action.actionTypeId() == ws2812fxPowerActionTypeId) {
//        command.append("b ");
//        if (action.param(ws2812fxPowerActionPowerParamTypeId).value().toBool()) {
//            command.append("30");
//        } else {
//            command.append("0");
//        }
//        command.append("\r\n");
//        return sendCommand(info, command, CommandType::Brightness);
//    }

//    if (action.actionTypeId() == ws2812fxBrightnessActionTypeId) {

//        command.append("b ");
//        command.append(action.param(ws2812fxBrightnessActionBrightnessParamTypeId).value().toString());
//        command.append("\r\n");
//        return sendCommand(info, command, CommandType::Brightness);
//    }

//    if (action.actionTypeId() == ws2812fxSpeedActionTypeId) {

//        command.append("s ");
//        command.append(action.param(ws2812fxSpeedActionSpeedParamTypeId).value().toString());
//        command.append("\r\n");
//        return sendCommand(info, command, CommandType::Speed);
//    }

//    if (action.actionTypeId() == ws2812fxColorActionTypeId) {

//        QColor color;
//        color= action.param(ws2812fxColorActionColorParamTypeId).value().value<QColor>();
//        command.append("c ");
//        command.append(QString(color.name()).remove("#"));
//        command.append("\r\n");
//        return sendCommand(info, command, CommandType::Color);
//    }

//    if (action.actionTypeId() == ws2812fxColorTemperatureActionTypeId) {

//        // minValue 153, maxValue 500
//        QColor color;
//        color.setRgb(255, 255, static_cast<int>((255.00-(((action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value().toDouble()-153.00)/347.00))*255.00)));
//        thing->setStateValue(ws2812fxColorTemperatureStateTypeId, action.param(ws2812fxColorTemperatureActionColorTemperatureParamTypeId).value());
//        command.append("c ");
//        command.append(QString(color.name()).remove("#"));
//        command.append("\r\n");
//        return sendCommand(info, command, CommandType::Color);
//    }

//    if (action.actionTypeId() == ws2812fxEffectModeActionTypeId) {

//        QString effectMode = action.param(ws2812fxEffectModeActionEffectModeParamTypeId).value().toString();
//        command.append("m ");
//        if (effectMode == "Static") {
//            command.append(QString::number(FX_MODE_STATIC));
//        } else if (effectMode == "Blink") {
//            command.append(QString::number(FX_MODE_BLINK));
//        } else if (effectMode == "Color Wipe") {
//            command.append(QString::number(FX_MODE_COLOR_WIPE));
//        } else if (effectMode == "Color Wipe Inverse") {
//            command.append(QString::number(FX_MODE_COLOR_WIPE_INV));
//        } else if (effectMode == "Color Wipe Reverse") {
//            command.append(QString::number(FX_MODE_COLOR_WIPE_REV));
//        } else if (effectMode == "Color Wipe Reverse Inverse") {
//            command.append(QString::number(FX_MODE_COLOR_WIPE_REV_INV));
//        } else if (effectMode == "Color Wipe Random") {
//            command.append(QString::number(FX_MODE_COLOR_WIPE_RANDOM));
//        } else if (effectMode == "Random Color") {
//            command.append(QString::number(FX_MODE_RANDOM_COLOR));
//        } else if (effectMode == "Single Dynamic") {
//            command.append(QString::number(FX_MODE_SINGLE_DYNAMIC));
//        } else if (effectMode == "Multi Dynamic") {
//            command.append(QString::number(FX_MODE_MULTI_DYNAMIC));
//        } else if (effectMode == "Rainbow") {
//            command.append(QString::number(FX_MODE_RAINBOW));
//        } else if (effectMode == "Rainbow Cycle") {
//            command.append(QString::number(FX_MODE_RAINBOW_CYCLE));
//        } else if (effectMode == "Scan") {
//            command.append(QString::number(FX_MODE_SCAN));
//        } else if (effectMode == "Dual Scan") {
//            command.append(QString::number(FX_MODE_DUAL_SCAN));
//        } else if (effectMode == "Fade") {
//            command.append(QString::number(FX_MODE_FADE));
//        } else if (effectMode == "Theater Chase") {
//            command.append(QString::number(FX_MODE_THEATER_CHASE));
//        } else if (effectMode == "Theater Chase Rainbow") {
//            command.append(QString::number(FX_MODE_THEATER_CHASE_RAINBOW));
//        } else if (effectMode == "Running Lights") {
//            command.append(QString::number(FX_MODE_RUNNING_LIGHTS));
//        } else if (effectMode == "Twinkle") {
//            command.append(QString::number(FX_MODE_TWINKLE));
//        } else if (effectMode == "Twinkle Random") {
//            command.append(QString::number(FX_MODE_TWINKLE_RANDOM));
//        } else if (effectMode == "Twinkle Fade") {
//            command.append(QString::number(FX_MODE_TWINKLE_FADE));
//        } else if (effectMode == "Twinkle Fade Random") {
//            command.append(QString::number(FX_MODE_TWINKLE_FADE_RANDOM));
//        } else if (effectMode == "Sparkle") {
//            command.append(QString::number(FX_MODE_SPARKLE));
//        } else if (effectMode == "Flash Sparkle") {
//            command.append(QString::number(FX_MODE_FLASH_SPARKLE));
//        } else if (effectMode == "Hyper Sparkle") {
//            command.append(QString::number(FX_MODE_HYPER_SPARKLE));
//        } else if (effectMode == "Strobe") {
//            command.append(QString::number(FX_MODE_STROBE));
//        } else if (effectMode == "Strobe Rainbow") {
//            command.append(QString::number(FX_MODE_STROBE_RAINBOW));
//        } else if (effectMode == "Multi Strobe") {
//            command.append(QString::number(FX_MODE_MULTI_STROBE));
//        } else if (effectMode == "Blink Rainbow") {
//            command.append(QString::number(FX_MODE_BLINK_RAINBOW));
//        } else if (effectMode == "Chase White") {
//            command.append(QString::number(FX_MODE_CHASE_WHITE));
//        } else if (effectMode == "Chase Color") {
//            command.append(QString::number(FX_MODE_CHASE_COLOR));
//        } else if (effectMode == "Chase Random") {
//            command.append(QString::number(FX_MODE_CHASE_RANDOM));
//        } else if (effectMode == "Chase Flash") {
//            command.append(QString::number(FX_MODE_CHASE_FLASH));
//        } else if (effectMode == "Chase Flash Random") {
//            command.append(QString::number(FX_MODE_CHASE_FLASH_RANDOM));
//        } else if (effectMode == "Chase Rainbow White") {
//            command.append(QString::number(FX_MODE_CHASE_RAINBOW_WHITE));
//        } else if (effectMode == "Chase Blackout") {
//            command.append(QString::number(FX_MODE_CHASE_BLACKOUT));
//        } else if (effectMode == "Chase Blackout Rainbow") {
//            command.append(QString::number(FX_MODE_CHASE_BLACKOUT_RAINBOW));
//        } else if (effectMode == "Color Sweep Random") {
//            command.append(QString::number(FX_MODE_COLOR_SWEEP_RANDOM));
//        } else if (effectMode == "Running Color") {
//            command.append(QString::number(FX_MODE_RUNNING_COLOR));
//        } else if (effectMode == "Running Red Blue") {
//            command.append(QString::number(FX_MODE_RUNNING_RED_BLUE));
//        } else if (effectMode == "Running Random") {
//            command.append(QString::number(FX_MODE_RUNNING_RANDOM));
//        }else if (effectMode == "Larson Scanner") {
//            command.append(QString::number(FX_MODE_LARSON_SCANNER));
//        }else if (effectMode == "Comet") {
//            command.append(QString::number(FX_MODE_COMET));
//        }else if (effectMode == "Fireworks") {
//            command.append(QString::number(FX_MODE_FIREWORKS));
//        }else if (effectMode == "Fireworks Random") {
//            command.append(QString::number(FX_MODE_FIREWORKS_RANDOM));
//        }else if (effectMode == "Merry Christmas") {
//            command.append(QString::number(FX_MODE_MERRY_CHRISTMAS));
//        }else if (effectMode == "Fire Flicker") {
//            command.append(QString::number(FX_MODE_FIRE_FLICKER));
//        }else if (effectMode == "Fire Flicker (soft)") {
//            command.append(QString::number(FX_MODE_FIRE_FLICKER_SOFT));
//        }else if (effectMode == "Fire Flicker (intense)") {
//            command.append(QString::number(FX_MODE_FIRE_FLICKER_INTENSE));
//        }else if (effectMode == "Circus Combustus") {
//            command.append(QString::number(FX_MODE_CIRCUS_COMBUSTUS));
//        }else if (effectMode == "Halloween") {
//            command.append(QString::number(FX_MODE_HALLOWEEN));
//        }else if (effectMode == "Bicolor Chase") {
//            command.append(QString::number(FX_MODE_BICOLOR_CHASE));
//        }else if (effectMode == "Tricolor Chase") {
//            command.append(QString::number(FX_MODE_TRICOLOR_CHASE));
//        }else if (effectMode == "ICU") {
//            command.append(QString::number(FX_MODE_ICU));
//        }else if (effectMode == "Custom 0") {
//            command.append(QString::number(FX_MODE_CUSTOM_0));
//        }else if (effectMode == "Custom 1") {
//            command.append(QString::number(FX_MODE_CUSTOM_1));
//        }else if (effectMode == "Custom 2") {
//            command.append(QString::number(FX_MODE_CUSTOM_2));
//        }else if (effectMode == "Custom 3") {
//            command.append(QString::number(FX_MODE_CUSTOM_3));
//        }
//        command.append("\r\n");
//        return sendCommand(info, command, CommandType::Mode);
//    }
}


void IntegrationPluginWs2812fx::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == ws2812fxThingClassId) {

        m_usedInterfaces.removeAll(thing->paramValue(ws2812fxThingSerialPortParamTypeId).toString());
        NymeaLight *light = m_lights.take(thing);
        light->deleteLater();
    }
}

