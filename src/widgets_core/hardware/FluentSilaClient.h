#ifndef FLUENTSILACLIENT_H
#define FLUENTSILACLIENT_H

#include <QString>
#include <QObject>
#include <QMap>
#include <QVariant>
#include <memory>

// gRPC headers
#include <grpcpp/grpcpp.h>

#include "SilaFluentController.grpc.pb.h"
#include "SilaFluentStatusProvider.grpc.pb.h"

class FluentSilaClient : public QObject {
    Q_OBJECT
public:
    explicit FluentSilaClient(const QString& address, QObject* parent = nullptr);
    ~FluentSilaClient();

    // Initialize the Fluent runtime (start or attach)
    bool startFluentOrAttach(QString& errorOut);

    // Sets a single variable on the Fluent instrument
    bool setVariableValue(const QString& variableName, const QString& variableValue, QString& errorOut);
    
    // Sets multiple variables on the Fluent instrument
    bool setVariables(const QMap<QString, QString>& vars, QString& errorOut);
    
    // Runs the experiment script on the Fluent instrument after setting variables
    bool experimentFromHelix(const QString& runId, const QMap<QString, QString>& vars, QString& errorOut);

signals:
    void stateChanged(const QString& stateString);

private:
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<sila2::tecan::fluent::silafluentcontroller::v1::SilaFluentController::Stub> m_stub;
    std::unique_ptr<sila2::tecan::fluent::silafluentstatusprovider::v1::SilaFluentStatusProvider::Stub> m_status_stub;
};

#endif // FLUENTSILACLIENT_H
