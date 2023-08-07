//This header file causes wierd problems in Visual Studio when included in subclasses of std::vector or std::map. Why?

#pragma once

#include <memory>
#include "EndFrameqct.h"
#include "AtPointConstraintIqctJqc.h"
#include "DirectionCosineConstraintIqctJqc.h"
#include "TranslationConstraintIqctJqc.h"
#include "DispCompIeqctJeqcKeqct.h"
#include "DispCompIeqctJeqcO.h"

namespace MbD {

	template<typename T>
	class CREATE {
	public:
		static std::shared_ptr<T> With(const char* name) {
			auto inst = std::make_shared<T>(name);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(const std::string& expr, double unit) {
			auto inst = std::make_shared<T>(expr, unit);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With() {
			auto inst = std::make_shared<T>();
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(int n) {
			auto inst = std::make_shared<T>(n);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(int m, int n) {
			auto inst = std::make_shared<T>(m, n);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(std::initializer_list<double> listD) {
			auto inst = std::make_shared<T>(listD);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj) {
			auto inst = std::make_shared<T>(frmi, frmj);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj, int axis) {
			auto inst = std::make_shared<T>(frmi, frmj, axis);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj, std::shared_ptr<EndFramec> frmk, int axisk) {
			auto inst = std::make_shared<T>(frmi, frmj, frmk, axisk);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<Constraint> ConstraintWith(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj, int axis) {
			std::shared_ptr<Constraint> inst;
			std::string str = typeid(T(frmi, frmj, axis)).name();
			if (str == "class MbD::AtPointConstraintIJ") {
				if (std::dynamic_pointer_cast<EndFrameqct>(frmi)) {
					inst = std::make_shared<AtPointConstraintIqctJqc>(frmi, frmj, axis);
				}
				else {
					inst = std::make_shared<AtPointConstraintIqcJqc>(frmi, frmj, axis);
				}
			}
			else if(str == "class MbD::TranslationConstraintIJ") {
				if (std::dynamic_pointer_cast<EndFrameqct>(frmi)) {
					inst = std::make_shared<TranslationConstraintIqctJqc>(frmi, frmj, axis);
				}
				else {
					inst = std::make_shared<TranslationConstraintIqcJqc>(frmi, frmj, axis);
				}
			}
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<T> With(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj, int axisi, int axisj) {
			auto inst = std::make_shared<T>(frmi, frmj, axisi, axisj);
			inst->initialize();
			return inst;
		}
		static std::shared_ptr<Constraint> ConstraintWith(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj, int axisi, int axisj) {
			std::shared_ptr<Constraint> inst;
			std::string str = typeid(T(frmi, frmj, axisi, axisj)).name();
			if (str == "class MbD::DirectionCosineConstraintIJ") {
				if (std::dynamic_pointer_cast<EndFrameqct>(frmi)) {
					inst = std::make_shared<DirectionCosineConstraintIqctJqc>(frmi, frmj, axisi, axisj);
				}
				else {
					inst = std::make_shared<DirectionCosineConstraintIqcJqc>(frmi, frmj, axisi, axisj);
				}
			}
			inst->initialize();
			return inst;
		}
	};
}

