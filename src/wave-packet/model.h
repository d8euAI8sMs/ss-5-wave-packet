#pragma once

#include <functional>
#include <vector>

#include <util/common/math/scalar.h>
#include <util/common/math/common.h>

namespace model
{

    using rfunc_t = std::function < double (double) > ;
    using cfunc_t = std::function < math::complex < double > (double) > ;

    inline rfunc_t make_potential_fn(double a, double v0, int l0)
    {
        return [=] (double x)
        {
            x = std::abs(x); // for testing purpose
            double orbital = 0;
            if (l0 != 0)
            {
                double x0 = max(x, 1e-3);
                orbital = l0 * (l0 + 1) / x0 / x0;
            }
            return ((x < a) ? (-v0) : 0) + orbital;
        };
    }

    inline cfunc_t make_free_space_condition_fn(double a, double gamma)
    {
        return [=] (double x) -> math::complex < double >
        {
            x = std::abs(x); // for testing purpose
            return (x < a)
                    ? 1
                    : (1 / (1 + math::_i * gamma * (x - a) * (x - a)));
        };
    }

    struct tmm_data
    {
        double dt, dx;
        std::vector < double > potential;
        std::vector < math::complex < double > > space_cond;
        std::vector < math::complex < double > > A, B, C;
    };

    inline void tmm_make_data(tmm_data & data,
                              double dt,
                              double x0, double x1, double dx,
                              rfunc_t & potential,
                              cfunc_t & space_cond)
    {
        size_t n = (size_t) std::floor((x1 - x0) / dx);
        data.A.resize(n); data.B.resize(n); data.C.resize(n);
        data.potential.resize(n); data.space_cond.resize(n);
        data.dt = dt; data.dx = dx;
        for (size_t k = 0; k < n; ++k)
        {
            data.space_cond[k] = space_cond(x0 + k * dx);
            data.potential[k] = potential(x0 + k * dx);
        }
        for (size_t k = 1; k < n - 1; ++k)
        {
            data.A[k] = - (math::_i * dt) * data.space_cond[k] * data.space_cond[k - 1] / dx / dx / 2;
            data.B[k] = - (math::_i * dt) * data.space_cond[k] * data.space_cond[k + 1] / dx / dx / 2;
            data.C[k] = 1 + (math::_i * dt) * data.potential[k] / 2
                          + (math::_i * dt) * data.space_cond[k]
                              * (data.space_cond[k + 1] + data.space_cond[k - 1]) / dx / dx / 2;
        }
    }

    inline void tmm_solve(tmm_data & data,
                          std::vector < math::complex < double > > & src,
                          std::vector < math::complex < double > > & dst,
                          math::complex < double > mu0,
                          math::complex < double > nu0,
                          math::complex < double > muN,
                          math::complex < double > nuN)
    {
        double dt = data.dt, dx = data.dx;
        math::complex < double > D;

        std::vector < math::complex < double > > a(src.size()), b(src.size());
        a[1] = mu0; b[1] = nu0;
        for (size_t k = 1; k < src.size() - 1; ++k)
        {
            D = src[k] + (math::_i * dt) / 2 *
                (
                    data.space_cond[k] *
                    (
                        data.space_cond[k + 1] * (src[k + 1] - src[k]) -
                        data.space_cond[k - 1] * (src[k] - src[k - 1])
                    ) / dx / dx - data.potential[k] * src[k]
                );
            a[k + 1] = - data.B[k] / (data.C[k] + data.A[k] * a[k]);
            b[k + 1] = (D - data.A[k] * b[k]) / (data.C[k] + data.A[k] * a[k]);
        }

        dst.resize(src.size());
        dst[src.size() - 1] = (muN * b[src.size() - 1] + nuN) / (1 - muN * a[src.size() - 1]);
        for (size_t k = src.size(); k-- > 1;)
        {
            dst[k - 1] = a[k] * dst[k] + b[k];
        }
    }
}
