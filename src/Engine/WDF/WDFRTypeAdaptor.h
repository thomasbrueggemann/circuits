#pragma once

#include "WDFCore.h"
#include <vector>
#include <cmath>

namespace WDF {

/**
 * Three-port series adaptor for convenience.
 * Connects three elements in series.
 */
class WDFSeriesAdaptor3 : public WDFAdaptor {
public:
    void connectChild(WDFElement* child) override {
        if (numConnected < 3) {
            children[numConnected++] = child;
            if (numConnected == 3) {
                calculatePortResistance();
            }
        }
    }
    
    void connectChildren(WDFElement* c1, WDFElement* c2, WDFElement* c3) {
        children[0] = c1;
        children[1] = c2;
        children[2] = c3;
        numConnected = 3;
        calculatePortResistance();
    }
    
    int getNumPorts() const override { return 3; }
    
    void calculatePortResistance() override {
        if (numConnected < 3) return;
        
        double R1 = children[0]->getPortResistance();
        double R2 = children[1]->getPortResistance();
        double R3 = children[2]->getPortResistance();
        R = R1 + R2 + R3;
        
        gamma[0] = R1 / R;
        gamma[1] = R2 / R;
        gamma[2] = R3 / R;
    }
    
    void propagateFromChildren() override {
        if (numConnected < 3) return;
        
        for (int i = 0; i < 3; ++i) {
            children[i]->propagate();
        }
        
        b = -(children[0]->getReflectedWave() + 
              children[1]->getReflectedWave() + 
              children[2]->getReflectedWave());
    }
    
    void scatterToChildren() override {
        if (numConnected < 3) return;
        
        double b0 = children[0]->getReflectedWave();
        double b1 = children[1]->getReflectedWave();
        double b2 = children[2]->getReflectedWave();
        double sum = a + b0 + b1 + b2;
        
        children[0]->setIncidentWave(a - gamma[0] * sum);
        children[1]->setIncidentWave(a - gamma[1] * sum);
        children[2]->setIncidentWave(a - gamma[2] * sum);
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
private:
    WDFElement* children[3] = {nullptr, nullptr, nullptr};
    double gamma[3] = {0.333, 0.333, 0.334};
    int numConnected = 0;
};

/**
 * Three-port parallel adaptor for convenience.
 */
class WDFParallelAdaptor3 : public WDFAdaptor {
public:
    void connectChild(WDFElement* child) override {
        if (numConnected < 3) {
            children[numConnected++] = child;
            if (numConnected == 3) {
                calculatePortResistance();
            }
        }
    }
    
    void connectChildren(WDFElement* c1, WDFElement* c2, WDFElement* c3) {
        children[0] = c1;
        children[1] = c2;
        children[2] = c3;
        numConnected = 3;
        calculatePortResistance();
    }
    
    int getNumPorts() const override { return 3; }
    
    void calculatePortResistance() override {
        if (numConnected < 3) return;
        
        double R1 = children[0]->getPortResistance();
        double R2 = children[1]->getPortResistance();
        double R3 = children[2]->getPortResistance();
        
        // Parallel combination: 1/R = 1/R1 + 1/R2 + 1/R3
        double G = 1.0/R1 + 1.0/R2 + 1.0/R3;
        R = 1.0 / G;
        
        gamma[0] = R / R1;
        gamma[1] = R / R2;
        gamma[2] = R / R3;
    }
    
    void propagateFromChildren() override {
        if (numConnected < 3) return;
        
        for (int i = 0; i < 3; ++i) {
            children[i]->propagate();
        }
        
        b = gamma[0] * children[0]->getReflectedWave() +
            gamma[1] * children[1]->getReflectedWave() +
            gamma[2] * children[2]->getReflectedWave();
    }
    
    void scatterToChildren() override {
        if (numConnected < 3) return;
        
        double b0 = children[0]->getReflectedWave();
        double b1 = children[1]->getReflectedWave();
        double b2 = children[2]->getReflectedWave();
        
        // Common voltage point
        double v = gamma[0]*b0 + gamma[1]*b1 + gamma[2]*b2;
        
        children[0]->setIncidentWave(a + v - b0);
        children[1]->setIncidentWave(a + v - b1);
        children[2]->setIncidentWave(a + v - b2);
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
private:
    WDFElement* children[3] = {nullptr, nullptr, nullptr};
    double gamma[3] = {0.333, 0.333, 0.334};
    int numConnected = 0;
};

/**
 * N-port series adaptor.
 * Connects N elements in series.
 */
class WDFSeriesAdaptorN : public WDFAdaptor {
public:
    void connectChild(WDFElement* child) override {
        children.push_back(child);
        calculatePortResistance();
    }
    
    void connectChildren(const std::vector<WDFElement*>& newChildren) {
        children = newChildren;
        calculatePortResistance();
    }
    
    int getNumPorts() const override { 
        return static_cast<int>(children.size()); 
    }
    
    void calculatePortResistance() override {
        if (children.empty()) return;
        
        R = 0.0;
        gamma.resize(children.size());
        
        for (auto* child : children) {
            R += child->getPortResistance();
        }
        
        for (size_t i = 0; i < children.size(); ++i) {
            gamma[i] = children[i]->getPortResistance() / R;
        }
    }
    
    void propagateFromChildren() override {
        if (children.empty()) return;
        
        b = 0.0;
        for (auto* child : children) {
            child->propagate();
            b -= child->getReflectedWave();
        }
    }
    
    void scatterToChildren() override {
        if (children.empty()) return;
        
        double sum = a;
        for (auto* child : children) {
            sum += child->getReflectedWave();
        }
        
        for (size_t i = 0; i < children.size(); ++i) {
            children[i]->setIncidentWave(a - gamma[i] * sum);
        }
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
private:
    std::vector<WDFElement*> children;
    std::vector<double> gamma;
};

/**
 * N-port parallel adaptor.
 * Connects N elements in parallel.
 */
class WDFParallelAdaptorN : public WDFAdaptor {
public:
    void connectChild(WDFElement* child) override {
        children.push_back(child);
        calculatePortResistance();
    }
    
    void connectChildren(const std::vector<WDFElement*>& newChildren) {
        children = newChildren;
        calculatePortResistance();
    }
    
    int getNumPorts() const override { 
        return static_cast<int>(children.size()); 
    }
    
    void calculatePortResistance() override {
        if (children.empty()) return;
        
        double G = 0.0;
        gamma.resize(children.size());
        
        for (auto* child : children) {
            G += 1.0 / child->getPortResistance();
        }
        R = 1.0 / G;
        
        for (size_t i = 0; i < children.size(); ++i) {
            gamma[i] = R / children[i]->getPortResistance();
        }
    }
    
    void propagateFromChildren() override {
        if (children.empty()) return;
        
        b = 0.0;
        for (size_t i = 0; i < children.size(); ++i) {
            children[i]->propagate();
            b += gamma[i] * children[i]->getReflectedWave();
        }
    }
    
    void scatterToChildren() override {
        if (children.empty()) return;
        
        // Common voltage point
        double v = 0.0;
        for (size_t i = 0; i < children.size(); ++i) {
            v += gamma[i] * children[i]->getReflectedWave();
        }
        
        for (size_t i = 0; i < children.size(); ++i) {
            children[i]->setIncidentWave(a + v - children[i]->getReflectedWave());
        }
    }
    
    void propagate() override {
        propagateFromChildren();
    }
    
private:
    std::vector<WDFElement*> children;
    std::vector<double> gamma;
};

} // namespace WDF
