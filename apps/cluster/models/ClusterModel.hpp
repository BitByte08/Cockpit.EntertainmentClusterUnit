#ifndef CLUSTER_CLUSTERMODEL_HPP
#define CLUSTER_CLUSTERMODEL_HPP

#include <QObject>
#include <memory>
#include "CANInterface.hpp"

struct can_frame;

class ClusterModel : public QObject {
    Q_OBJECT
public:
    explicit ClusterModel(QObject *parent = nullptr);
    ~ClusterModel() override = default;

    void setCANInterface(std::unique_ptr<CANInterface> can_interface);
    void startReceiving();
    void stopReceiving();

    // Getters for cluster data
    int getSpeed() const { return speed_; }
    int getRpm() const { return rpm_; }
    int getFuelLevel() const { return fuel_level_; }
    int getTemperature() const { return temperature_; }

signals:
    void speedChanged(int speed);
    void rpmChanged(int rpm);
    void fuelLevelChanged(int fuel);
    void temperatureChanged(int temp);

private slots:
    void onFrameReceived(const can_frame &frame);

private:
    void parseCANFrame(const can_frame &frame);

    std::unique_ptr<CANInterface> can_interface_;
    
    // Cluster data
    int speed_{0};
    int rpm_{0};
    int fuel_level_{100};
    int temperature_{90};
};

#endif // CLUSTER_CLUSTERMODEL_HPP
