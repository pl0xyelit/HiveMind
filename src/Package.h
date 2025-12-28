#pragma once

class Package {
public:
    Package(int id, int destX, int destY, int reward, int deadline)
        : id(id), destX(destX), destY(destY), reward(reward), deadline(deadline), delivered(false), delivered_tick(-1) {}

    int getId() const { return id; }
    int getDestX() const { return destX; }
    int getDestY() const { return destY; }
    int getReward() const { return reward; }
    int getDeadline() const { return deadline; }

    bool isDelivered() const { return delivered; }
    void markDelivered(int tick) { delivered = true; delivered_tick = tick; }
    int deliveredAt() const { return delivered_tick; }

private:
    int id;
    int destX, destY;
    int reward;
    int deadline;
    bool delivered;
    int delivered_tick;
};