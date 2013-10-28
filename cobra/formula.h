/*
 * Copyright 2013, Mirek Klimos <myreggg@gmail.com>
 */
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <string>
#include "./util.h"
#include "./ast-manager.h"

#ifndef COBRA_FORMULA_H_
#define COBRA_FORMULA_H_

class Formula;
class Variable;
class OrOperator;
class AndOperator;
class NotOperator;
typedef std::vector<Formula*> FormulaVec;

/* Prepositional formula
 */
class Formula: public Construct {
  /* variable (possibly negated) that is equivalent to this subformula,
   * used for Tseitin transformation
   */
  Variable* tseitin_var_ = nullptr;

 public:
  explicit Formula(int childCount)
      : Construct(childCount) { }

  virtual Formula* tseitin_var();
  virtual Formula* neg();
  virtual void dump(int indent = 0);
  virtual std::string pretty() = 0;

  virtual void TseitinTransformation(FormulaVec& clauses) = 0;
  virtual bool isSimple() { return false; }
  virtual AndOperator* ToCnf();
};

/* Base class for associative n-ary operator. Abstract.
 */
class NaryOperator: public Formula {
 protected:
  FormulaVec children_;

 public:
  NaryOperator()
      : Formula(1) {}

  NaryOperator(Formula* left, Formula* right)
      : Formula(1) {
    addChild(left);
    addChild(right);
  }

  explicit NaryOperator(FormulaVec* list)
      : Formula(1) {
    children_.insert(children_.end(), list->begin(), list->end());
    delete list;
  }

  explicit NaryOperator(FormulaVec& list)
      : Formula(1) {
    children_.insert(children_.end(), list.begin(), list.end());
  }

  void addChild(Formula* child) {
    children_.push_back(child);
  }

  void addChildren(FormulaVec& children) {
    children_.insert(children_.end(), children.begin(), children.end());
  }

  void removeChild(int nth) {
    children_[nth] = children_.back();
    children_.pop_back();
  }

  virtual Construct* child(uint nth) {
    assert(nth < children_.size());
    return children_[nth];
  }

  FormulaVec& children() {
    return children_;
  }

  virtual uint child_count() {
    return children_.size();
  }

 protected:
  std::string pretty_join(std::string sep) {
    if (children_.empty()) return "()";
    std::string s = "(" + children_.front()->pretty();
    for (auto it = std::next(children_.begin()); it != children_.end(); ++it) {
      s += sep;
      s += (*it)->pretty();
    }
    s += ")";
    return s;
  }
};

// n-ary operator AND
class AndOperator: public NaryOperator {
 public:
  AndOperator()
      : NaryOperator() { }
  AndOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) { }
  explicit AndOperator(FormulaVec* list)
      : NaryOperator(list) { }
  explicit AndOperator(FormulaVec list)
      : NaryOperator(list) { }

  virtual std::string name() {
    return "AndOperator";
  }

  virtual std::string pretty() {
    return pretty_join(" & ");
  }

  virtual Construct* Simplify();
  virtual void TseitinTransformation(FormulaVec& clauses);
};

// n-ary operator AND
class OrOperator: public NaryOperator {
 public:
  OrOperator(Formula *left, Formula* right)
      : NaryOperator(left, right) {}
  explicit OrOperator(FormulaVec* list)
      : NaryOperator(list) {}
  explicit OrOperator(FormulaVec list)
      : NaryOperator(list) {}

  virtual std::string name() {
    return "OrOperator";
  }

  virtual std::string pretty() {
    return pretty_join(" | ");
  }

  virtual Construct* Simplify();
  virtual void TseitinTransformation(FormulaVec& clauses);
};

/* Base class for AtLeast/AtMost/ExactlyOperator.
 */
class MacroOperator: public NaryOperator {
 protected:
  AndOperator* expanded_;
 public:
  explicit MacroOperator(FormulaVec* list);

  virtual Formula* tseitin_var();

  void ExpandHelper(int size, bool negate);
  virtual AndOperator* Expand() = 0;
  virtual void TseitinTransformation(FormulaVec& clauses);
};

// At least value_ children are satisfied
class AtLeastOperator: public MacroOperator {
  int value_;

 public:
  AtLeastOperator(int value, FormulaVec* list)
      : MacroOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "AtLeastOperator(" + to_string(value_) + ")";
  }

  virtual std::string pretty() {
    return "AtLeast-" + to_string(value_) + pretty_join(", ");
  }

  virtual AndOperator* Expand();
};

// At most value_ children are satisfied
class AtMostOperator: public MacroOperator {
  int value_;

 public:
  AtMostOperator(int value, FormulaVec* list)
      : MacroOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "AtMostOperator(" + to_string(value_) + ")";
  }

  virtual std::string pretty() {
    return "AtMost-" + to_string(value_) + pretty_join(", ");
  }

  virtual AndOperator* Expand();
};

// Exactly value_ of children are satisfied
class ExactlyOperator: public MacroOperator {
  int value_;

 public:
  ExactlyOperator(int value, FormulaVec* list)
      : MacroOperator(list),
        value_(value) { }

  virtual std::string name() {
    return "ExactlyOperator(" + to_string(value_) + ")";
  }

  virtual std::string pretty() {
    return "Exactly-" + to_string(value_) + pretty_join(", ");
  }

  virtual AndOperator* Expand();
};

// Equivalence aka iff
class EquivalenceOperator: public Formula {
  Formula* left_;
  Formula* right_;

 public:
  EquivalenceOperator(Formula* left, Formula* right)
      : Formula(2),
        left_(left),
        right_(right) { }

  virtual std::string name() {
    return "EquivalenceOperator";
  }

  virtual std::string pretty() {
    return "(" + left_->pretty() + " <-> " + right_->pretty() + ")";
  }

  virtual Construct* child(uint nth) {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

// Implication
class ImpliesOperator: public Formula {
  Formula* left_;
  Formula* right_;

 public:
  ImpliesOperator(Formula* premise, Formula* consequence)
      : Formula(2),
        left_(premise),
        right_(consequence) { }

  virtual std::string name() {
    return "ImpliesOperator";
  }

  virtual std::string pretty() {
    return "(" + left_->pretty() + " -> " + right_->pretty() + ")";
  }

  virtual Construct* child(uint nth) {
    assert(nth < child_count());
    switch (nth) {
      case 0: return left_;
      case 1: return right_;
      default: assert(false);
    }
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

// Simple negation.
class NotOperator: public Formula {
  Formula* child_;
 public:
  explicit NotOperator(Formula* child)
      : Formula(1),
        child_(child) { }

  virtual std::string name() {
    return "NotOperator";
  }

  virtual std::string pretty() {
    return "!" + child_->pretty();
  }

  virtual Construct* child(uint nth) {
    assert(nth == 0);
    return child_;
  }

  virtual Formula* neg() {
    return child_;
  }

  virtual bool isSimple() {
    return child_->isSimple();
  }

  virtual void TseitinTransformation(FormulaVec& clauses);
};

// Just a variable, the only possible leaf
class Variable: public Formula {
  std::string ident_;
  bool generated_;
  int id_;

  static int id_counter_;

 public:
  /*  */
  Variable()
      : Formula(0),
        generated_(true) {
    id_ = id_counter_++;
    ident_ = "var" + to_string(id_);
  }

  explicit Variable(std::string ident)
      : Formula(0),
        ident_(ident),
        generated_(false) {
    id_ = id_counter_++;
  }

  int id() {
    return id_;
  }

  virtual std::string ident() {
    return ident_;
  }

  virtual std::string pretty() {
    return ident_;
  }

  virtual std::string name() {
    return "Variable " + ident();
  }

  virtual Construct* child(uint nth) {
    assert(false);
  }

  virtual bool isSimple() {
    return true;
  }

  virtual void TseitinTransformation(FormulaVec& clauses) {
    // do nothing
  }
};

extern AstManager m;

#endif  // COBRA_FORMULA_H_
