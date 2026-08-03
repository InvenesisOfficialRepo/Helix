#include "FluentSilaClient.h"
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include "SilaFluentController.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using sila2::tecan::fluent::silafluentcontroller::v1::SilaFluentController;
using sila2::tecan::fluent::silafluentcontroller::v1::SetVariableValue_Parameters;
using sila2::tecan::fluent::silafluentcontroller::v1::SetVariableValue_Responses;
using sila2::org::silastandard::String;

FluentSilaClient::FluentSilaClient(const QString& address, QObject* parent) 
    : QObject(parent)
{
    qDebug() << "Connecting to SiLA 2 Fluent server at:" << address;
    m_channel = grpc::CreateChannel(address.toStdString(), grpc::InsecureChannelCredentials());
    m_stub = SilaFluentController::NewStub(m_channel);
    m_status_stub = sila2::tecan::fluent::silafluentstatusprovider::v1::SilaFluentStatusProvider::NewStub(m_channel);
}

FluentSilaClient::~FluentSilaClient() = default;

bool FluentSilaClient::startFluentOrAttach(QString& errorOut)
{
    if (!m_stub) {
        errorOut = "SiLA client not initialized.";
        return false;
    }

    using sila2::tecan::fluent::silafluentcontroller::v1::StartFluentOrAttach_Parameters;
    using sila2::tecan::fluent::silafluentcontroller::v1::StartFluentOrAttach_Responses;

    StartFluentOrAttach_Parameters params;
    StartFluentOrAttach_Responses response;
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(60)); // Long timeout for startup

    Status status = m_stub->StartFluentOrAttach(&context, params, &response);

    if (status.ok()) {
        return true;
    } else {
        errorOut = QString::fromStdString(status.error_message());
        qWarning() << "StartFluentOrAttach failed:" << errorOut;
        return false;
    }
}

bool FluentSilaClient::setVariableValue(const QString& variableName, const QString& variableValue, QString& errorOut)
{
    if (!m_stub) {
        errorOut = "SiLA client not initialized.";
        return false;
    }

    SetVariableValue_Parameters params;
    
    // Allocate the dynamic messages
    String* varNameMsg = new String();
    varNameMsg->set_value(variableName.toStdString());
    params.set_allocated_variablename(varNameMsg);

    String* varValMsg = new String();
    varValMsg->set_value(variableValue.toStdString());
    params.set_allocated_value(varValMsg);

    SetVariableValue_Responses response;
    ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));

    Status status = m_stub->SetVariableValue(&context, params, &response);

    if (status.ok()) {
        return true;
    } else {
        errorOut = QString::fromStdString(status.error_message());
        qWarning() << "SetVariableValue failed:" << errorOut;
        return false;
    }
}

bool FluentSilaClient::setVariables(const QMap<QString, QString>& vars, QString& errorOut)
{
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        qDebug() << "Pushing SiLA Variable:" << it.key() << "=" << it.value();
        if (!setVariableValue(it.key(), it.value(), errorOut)) {
            errorOut = QString("Failed to set variable '%1': %2").arg(it.key(), errorOut);
            return false;
        }

        // Verify it was set
        using sila2::tecan::fluent::silafluentcontroller::v1::GetVariableValue_Parameters;
        using sila2::tecan::fluent::silafluentcontroller::v1::GetVariableValue_Responses;
        GetVariableValue_Parameters getParams;
        getParams.set_allocated_variablename(new sila2::org::silastandard::String());
        getParams.mutable_variablename()->set_value(it.key().toStdString());
        GetVariableValue_Responses getResp;
        ClientContext getCtx;
        getCtx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
        Status getStatus = m_stub->GetVariableValue(&getCtx, getParams, &getResp);
        if (getStatus.ok()) {
            qDebug() << "  -> Server reports value is now:" << QString::fromStdString(getResp.returnvalue().value());
        } else {
            qDebug() << "  -> Failed to read back value:" << QString::fromStdString(getStatus.error_message());
        }
    }
    return true;
}

bool FluentSilaClient::experimentFromHelix(const QString& runId, const QMap<QString, QString>& vars, QString& errorOut)
{
    if (!m_stub) {
        errorOut = "SiLA client not initialized.";
        return false;
    }

    using sila2::tecan::fluent::silafluentcontroller::v1::PrepareMethod_Parameters;
    using sila2::tecan::fluent::silafluentcontroller::v1::PrepareMethod_Responses;
    using sila2::tecan::fluent::silafluentcontroller::v1::RunMethod_Parameters;
    using sila2::tecan::fluent::silafluentcontroller::v1::RunMethod_Responses;

    // 1. Prepare Method "ExperimentFromHelix"
    PrepareMethod_Parameters prepParams;
    String* methodMsg = new String();
    methodMsg->set_value("ExperimentFromHelix"); 
    prepParams.set_allocated_toprepare(methodMsg);

    PrepareMethod_Responses prepResponse;
    ClientContext prepContext;
    prepContext.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(30));

    Status prepStatus = m_stub->PrepareMethod(&prepContext, prepParams, &prepResponse);
    if (!prepStatus.ok()) {
        errorOut = "PrepareMethod failed: " + QString::fromStdString(prepStatus.error_message());
        qWarning() << errorOut;
        return false;
    }

    auto logState = [](const QString& phase, const QString& stateStr) {
        QFile logFile(QCoreApplication::applicationDirPath() + "/fluent_sila_states.log");
        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << " | " << phase << " | " << stateStr << "\n";
        }
    };

    // Wait for the method to be fully prepared (up to 15 seconds)
    using sila2::tecan::fluent::silafluentstatusprovider::v1::Subscribe_State_Parameters;
    using sila2::tecan::fluent::silafluentstatusprovider::v1::Subscribe_State_Responses;

    if (m_status_stub) {
        Subscribe_State_Parameters stateParams;
        ClientContext stateContext;
        // 15 seconds deadline for preparation
        gpr_timespec deadline = gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_seconds(15, GPR_TIMESPAN));
        stateContext.set_deadline(deadline);
        
        std::unique_ptr<grpc::ClientReader<Subscribe_State_Responses>> reader(
            m_status_stub->Subscribe_State(&stateContext, stateParams));

        Subscribe_State_Responses stateResp;
        while (reader->Read(&stateResp)) {
            QString stateStr = QString::fromStdString(stateResp.state().fluentcontrolstate().value());
            logState("PREPARE", stateStr);
            emit stateChanged("Preparing... [" + stateStr + "]");
            qDebug() << "Pre-Run State:" << stateStr;

            // Break if the state looks ready to run
            if (stateStr.contains("Idle", Qt::CaseInsensitive) || 
                stateStr.contains("Ready", Qt::CaseInsensitive) ||
                stateStr.contains("Prepared", Qt::CaseInsensitive) ||
                stateStr.contains("PreparingRun", Qt::CaseInsensitive) ||
                stateStr.compare("EditMode", Qt::CaseInsensitive) == 0) {
                // Give it one more tiny sleep just to be safe
                QThread::sleep(2);
                stateContext.TryCancel(); // Prevent reader destruction from hanging!
                break;
            }
        }
    } else {
        QThread::sleep(5); // fallback
    }

    // --- DEBUG: GET VARIABLE NAMES ---
    {
        using sila2::tecan::fluent::silafluentcontroller::v1::GetVariableNames_Parameters;
        using sila2::tecan::fluent::silafluentcontroller::v1::GetVariableNames_Responses;
        GetVariableNames_Parameters getVarParams;
        GetVariableNames_Responses getVarResp;
        ClientContext getVarCtx;
        getVarCtx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
        Status varStatus = m_stub->GetVariableNames(&getVarCtx, getVarParams, &getVarResp);
        if (varStatus.ok()) {
            QStringList availableVars;
            for (int i = 0; i < getVarResp.returnvalue_size(); ++i) {
                availableVars << QString::fromStdString(getVarResp.returnvalue(i).value());
            }
            qDebug() << "AVAILABLE SILA VARIABLES:" << availableVars;
            logState("SiLA Vars", availableVars.join(", "));
        } else {
            qWarning() << "Failed to GetVariableNames:" << QString::fromStdString(varStatus.error_message());
        }
    }

    // --- PUSH VARIABLES HERE (AFTER METHOD IS PREPARED) ---
    emit stateChanged("Pushing SiLA Variables...");
    if (!setVariables(vars, errorOut)) {
        qWarning() << "Failed to push variables after PrepareMethod:" << errorOut;
        return false;
    }

    // 2. Run the Method
    emit stateChanged("Starting RunMethod...");
    RunMethod_Parameters runParams;
    RunMethod_Responses runResponse;
    ClientContext runContext;
    
    Status runStatus = m_stub->RunMethod(&runContext, runParams, &runResponse);
    if (!runStatus.ok()) {
        errorOut = "RunMethod failed: " + QString::fromStdString(runStatus.error_message());
        qWarning() << errorOut;
        return false;
    }

    // 3. Monitor the execution
    if (m_status_stub) {
        Subscribe_State_Parameters stateParams;
        ClientContext stateContext;
        // No deadline, wait for the stream to finish
        std::unique_ptr<grpc::ClientReader<Subscribe_State_Responses>> reader(
            m_status_stub->Subscribe_State(&stateContext, stateParams));

        Subscribe_State_Responses stateResp;
        bool hasBeenRunning = false;
        
        qDebug() << "Monitoring execution state...";
        while (reader->Read(&stateResp)) {
            QString stateStr = QString::fromStdString(stateResp.state().fluentcontrolstate().value());
            logState("EXECUTE", stateStr);
            emit stateChanged("Execution: [" + stateStr + "]");
            qDebug() << "Fluent State Update:" << stateStr;

            if (stateStr.contains("Running", Qt::CaseInsensitive) || 
                stateStr.contains("Busy", Qt::CaseInsensitive) ||
                stateStr.contains("Executing", Qt::CaseInsensitive) ||
                stateStr.contains("BeginRun", Qt::CaseInsensitive) ||
                stateStr.contains("WaitingForSystem", Qt::CaseInsensitive) ||
                stateStr.contains("ResumeConditionCheck", Qt::CaseInsensitive)) {
                hasBeenRunning = true;
            } else if (stateStr.contains("Idle", Qt::CaseInsensitive) || 
                       stateStr.contains("Ready", Qt::CaseInsensitive) ||
                       stateStr.contains("RunFinished", Qt::CaseInsensitive) ||
                       stateStr.compare("EditMode", Qt::CaseInsensitive) == 0) {
                // If it was running and is now idle, RunFinished, or in EditMode, it finished successfully.
                if (hasBeenRunning) {
                    qDebug() << "Execution finished successfully.";
                    stateContext.TryCancel(); // Prevent reader destruction from hanging!
                    break;
                }
            } else if (stateStr.contains("Error", Qt::CaseInsensitive) || 
                       stateStr.contains("Aborted", Qt::CaseInsensitive) || 
                       stateStr.contains("Stopped", Qt::CaseInsensitive)) {
                errorOut = "Experiment finished with error state: " + stateStr;
                qWarning() << errorOut;
                stateContext.TryCancel(); // Prevent reader destruction from hanging!
                return false;
            }
        }
    }

    return true;
}
