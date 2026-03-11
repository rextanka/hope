// Builtins.cpp — additional built-in function registrations.
//
// The primary built-ins (arithmetic, comparison, boolean, list primitives)
// are registered in Evaluator::init_builtins().  This file registers any
// supplementary functions that didn't fit there.

#include "builtins/Builtins.hpp"
#include "runtime/Evaluator.hpp"
#include "runtime/RuntimeError.hpp"
#include "runtime/Value.hpp"

#include <cmath>

namespace hope {

void Builtins::init(Evaluator& ev) {
    // Transcendental functions.  All built-ins capture ev by reference so
    // that they can call ev.force() to unwrap lazy arguments.

    ev.register_builtin("sin", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            return make_num(std::sin(vn->n));
        }
        throw RuntimeError("sin: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("cos", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            return make_num(std::cos(vn->n));
        }
        throw RuntimeError("cos: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("tan", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            return make_num(std::tan(vn->n));
        }
        throw RuntimeError("tan: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("exp", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            return make_num(std::exp(vn->n));
        }
        throw RuntimeError("exp: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("log", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            return make_num(std::log(vn->n));
        }
        throw RuntimeError("log: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    // Inverse trig (1-arg)
    ev.register_builtin("asin", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::asin(vn->n));
        throw RuntimeError("asin: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("acos", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::acos(vn->n));
        throw RuntimeError("acos: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("atan", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::atan(vn->n));
        throw RuntimeError("atan: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    // Hyperbolic (1-arg)
    ev.register_builtin("sinh", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::sinh(vn->n));
        throw RuntimeError("sinh: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("cosh", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::cosh(vn->n));
        throw RuntimeError("cosh: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("tanh", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::tanh(vn->n));
        throw RuntimeError("tanh: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("asinh", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::asinh(vn->n));
        throw RuntimeError("asinh: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("acosh", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::acosh(vn->n));
        throw RuntimeError("acosh: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("atanh", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::atanh(vn->n));
        throw RuntimeError("atanh: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    // log10, erf, erfc (1-arg)
    ev.register_builtin("log10", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::log10(vn->n));
        throw RuntimeError("log10: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("erf", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::erf(vn->n));
        throw RuntimeError("erf: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("erfc", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::erfc(vn->n));
        throw RuntimeError("erfc: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    // ceil: alias for ceiling (Standard.hop declares both)
    ev.register_builtin("ceil", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) return make_num(std::ceil(vn->n));
        throw RuntimeError("ceil: expected a number", SourceLocation{"<runtime>", 0, 0, 0});
    });

    // atan2: takes a pair (y, x)
    ev.register_builtin("atan2", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        auto* pr = std::get_if<VPair>(&f->data);
        if (!pr) throw RuntimeError("atan2: expected a pair", SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fa = ev.force(pr->left);
        ValRef fb = ev.force(pr->right);
        auto* va = std::get_if<VNum>(&fa->data);
        auto* vb = std::get_if<VNum>(&fb->data);
        if (!va || !vb) throw RuntimeError("atan2: expected numbers", SourceLocation{"<runtime>", 0, 0, 0});
        return make_num(std::atan2(va->n, vb->n));
    });

    // hypot: takes a pair (x, y)
    ev.register_builtin("hypot", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        auto* pr = std::get_if<VPair>(&f->data);
        if (!pr) throw RuntimeError("hypot: expected a pair", SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fa = ev.force(pr->left);
        ValRef fb = ev.force(pr->right);
        auto* va = std::get_if<VNum>(&fa->data);
        auto* vb = std::get_if<VNum>(&fb->data);
        if (!va || !vb) throw RuntimeError("hypot: expected numbers", SourceLocation{"<runtime>", 0, 0, 0});
        return make_num(std::hypot(va->n, vb->n));
    });

    // Power operator: ** takes a pair (base, exponent)
    ev.register_builtin("**", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        auto* pr = std::get_if<VPair>(&f->data);
        if (!pr) {
            throw RuntimeError("**: expected a pair",
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        ValRef fa = ev.force(pr->left);
        ValRef fb = ev.force(pr->right);
        auto* va = std::get_if<VNum>(&fa->data);
        auto* vb = std::get_if<VNum>(&fb->data);
        if (!va || !vb) {
            throw RuntimeError("**: expected numbers",
                               SourceLocation{"<runtime>", 0, 0, 0});
        }
        return make_num(std::pow(va->n, vb->n));
    });

    // max / min over a pair
    ev.register_builtin("max", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        auto* pr = std::get_if<VPair>(&f->data);
        if (!pr) throw RuntimeError("max: expected pair",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fa = ev.force(pr->left);
        ValRef fb = ev.force(pr->right);
        auto* va = std::get_if<VNum>(&fa->data);
        auto* vb = std::get_if<VNum>(&fb->data);
        if (!va || !vb) throw RuntimeError("max: expected numbers",
                                           SourceLocation{"<runtime>", 0, 0, 0});
        return make_num(va->n >= vb->n ? va->n : vb->n);
    });

    ev.register_builtin("min", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        auto* pr = std::get_if<VPair>(&f->data);
        if (!pr) throw RuntimeError("min: expected pair",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fa = ev.force(pr->left);
        ValRef fb = ev.force(pr->right);
        auto* va = std::get_if<VNum>(&fa->data);
        auto* vb = std::get_if<VNum>(&fb->data);
        if (!va || !vb) throw RuntimeError("min: expected numbers",
                                           SourceLocation{"<runtime>", 0, 0, 0});
        return make_num(va->n <= vb->n ? va->n : vb->n);
    });

    // even / odd
    ev.register_builtin("even", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            long long n = static_cast<long long>(vn->n);
            return make_bool(n % 2 == 0);
        }
        throw RuntimeError("even: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    ev.register_builtin("odd", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        if (auto* vn = std::get_if<VNum>(&f->data)) {
            long long n = static_cast<long long>(vn->n);
            return make_bool(n % 2 != 0);
        }
        throw RuntimeError("odd: expected a number",
                           SourceLocation{"<runtime>", 0, 0, 0});
    });

    // gcd / lcm — take a pair
    ev.register_builtin("gcd", [&ev](ValRef v) -> ValRef {
        ValRef f = ev.force(v);
        auto* pr = std::get_if<VPair>(&f->data);
        if (!pr) throw RuntimeError("gcd: expected pair",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fa = ev.force(pr->left);
        ValRef fb = ev.force(pr->right);
        auto* va = std::get_if<VNum>(&fa->data);
        auto* vb = std::get_if<VNum>(&fb->data);
        if (!va || !vb) throw RuntimeError("gcd: expected numbers",
                                           SourceLocation{"<runtime>", 0, 0, 0});
        long long a = std::abs(static_cast<long long>(va->n));
        long long b = std::abs(static_cast<long long>(vb->n));
        while (b != 0) { long long t = b; b = a % b; a = t; }
        return make_num(static_cast<double>(a));
    });

    // length : list alpha -> num
    ev.register_builtin("length", [&ev](ValRef v) -> ValRef {
        long long n = 0;
        ValRef cur = ev.force(v);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ++n;
            ValRef arg = ev.force(vc->arg);
            auto* pr = std::get_if<VPair>(&arg->data);
            if (!pr) break;
            cur = ev.force(pr->right);
        }
        return make_num(static_cast<double>(n));
    });

    // reverse : list alpha -> list alpha
    ev.register_builtin("reverse", [&ev](ValRef v) -> ValRef {
        std::vector<ValRef> elems;
        ValRef cur = ev.force(v);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ValRef arg = ev.force(vc->arg);
            auto* pr = std::get_if<VPair>(&arg->data);
            if (!pr) break;
            elems.push_back(pr->left);
            cur = ev.force(pr->right);
        }
        ValRef result = make_nil();
        for (const auto& e : elems) {
            result = make_cons(e, result);
        }
        return result;
    });

    // map : (alpha -> beta) # list alpha -> list beta
    ev.register_builtin("map", [&ev](ValRef v) -> ValRef {
        ValRef fv = ev.force(v);
        auto* pr = std::get_if<VPair>(&fv->data);
        if (!pr) throw RuntimeError("map: expected (f, list)",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fn  = ev.force(pr->left);
        ValRef lst = pr->right;

        std::vector<ValRef> results;
        ValRef cur = ev.force(lst);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ValRef arg = ev.force(vc->arg);
            auto* lpr = std::get_if<VPair>(&arg->data);
            if (!lpr) break;
            ValRef mapped = ev.apply(fn, lpr->left);
            results.push_back(mapped);
            cur = ev.force(lpr->right);
        }
        ValRef result = make_nil();
        for (auto it = results.rbegin(); it != results.rend(); ++it) {
            result = make_cons(*it, result);
        }
        return result;
    });

    // filter : (alpha -> bool) # list alpha -> list alpha
    ev.register_builtin("filter", [&ev](ValRef v) -> ValRef {
        ValRef fv = ev.force(v);
        auto* pr = std::get_if<VPair>(&fv->data);
        if (!pr) throw RuntimeError("filter: expected (pred, list)",
                                    SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fn  = ev.force(pr->left);
        ValRef lst = pr->right;

        std::vector<ValRef> results;
        ValRef cur = ev.force(lst);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ValRef arg = ev.force(vc->arg);
            auto* lpr = std::get_if<VPair>(&arg->data);
            if (!lpr) break;
            ValRef head = lpr->left;
            ValRef test = ev.force(ev.apply(fn, head));
            auto* tc = std::get_if<VCons>(&test->data);
            if (tc && tc->name == "true") {
                results.push_back(head);
            }
            cur = ev.force(lpr->right);
        }
        ValRef result = make_nil();
        for (auto it = results.rbegin(); it != results.rend(); ++it) {
            result = make_cons(*it, result);
        }
        return result;
    });

    // foldl : (beta # alpha -> beta) # beta # list alpha -> beta
    // In Hope: foldl f z [] = z; foldl f z (x:xs) = foldl f (f(z,x)) xs
    // As a builtin taking a triple: foldl (f, z, list)
    ev.register_builtin("foldl", [&ev](ValRef v) -> ValRef {
        ValRef fv = ev.force(v);
        // Expect (f, z, list) as right-nested pair: (f, (z, list))
        auto* pr1 = std::get_if<VPair>(&fv->data);
        if (!pr1) throw RuntimeError("foldl: expected (f, z, list)",
                                     SourceLocation{"<runtime>", 0, 0, 0});
        ValRef fn = ev.force(pr1->left);
        ValRef rest = ev.force(pr1->right);
        auto* pr2 = std::get_if<VPair>(&rest->data);
        if (!pr2) throw RuntimeError("foldl: expected (f, z, list)",
                                     SourceLocation{"<runtime>", 0, 0, 0});
        ValRef acc = pr2->left;
        ValRef lst = pr2->right;

        ValRef cur = ev.force(lst);
        while (true) {
            auto* vc = std::get_if<VCons>(&cur->data);
            if (!vc || vc->name == "nil") break;
            if (vc->name != "::") break;
            ValRef arg = ev.force(vc->arg);
            auto* lpr = std::get_if<VPair>(&arg->data);
            if (!lpr) break;
            ValRef head = lpr->left;
            ValRef pair_arg = make_pair(acc, head);
            acc = ev.apply(fn, pair_arg);
            cur = ev.force(lpr->right);
        }
        return acc;
    });
}

} // namespace hope
