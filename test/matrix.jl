#TODO: basic transform matrices.


# Test a hypothetical camera's view matrix.
const VIEW_POS = v3f(4.5, -5, 10)
const VIEW_FORWARD = vnorm(v3f(1, 1, 1))
const VIEW_UP = v3f(0, 0, 1)
const VIEW_ORTHO_RIGHT = vbasis(VIEW_FORWARD, VIEW_UP).right
const VIEW_ORTHO_UP = v_rightward(VIEW_ORTHO_RIGHT, VIEW_FORWARD)
const VIEW_MAT = m4_look_at(VIEW_POS, VIEW_POS + VIEW_FORWARD, VIEW_UP)
function test_point(input::v3f, expected::v3f, atol = Float32(0.0001))
    @bp_test_no_allocations(
        begin
            precise_result = m_apply_point(VIEW_MAT, input)::v3f
            rounded_result = map(round, precise_result / atol) * atol
            rounded_result
        end,
        expected,
        "Input: ", input, "\n\t",
        "View matrix:[", join(
            string("\n\t\t", join(VIEW_MAT[row, :], ", "))
              for row in 1:size(VIEW_MAT, 1)
        ), "\n\t]"
    )
end
test_point(VIEW_POS + VIEW_FORWARD, v3f(0, 0, -1))
test_point(VIEW_POS + VIEW_ORTHO_RIGHT, v3f(1, 0, 0))
test_point(VIEW_POS + VIEW_ORTHO_UP, v3f(0, 1, 0))

# Test m3_look_at(from_basis, to_basis).
BASIS_FROM::VBasis{Float32} = vbasis(vnorm(v3f(1, 4, -1.75)),
                                     v3f(0, 0, 1))
BASIS_TO::VBasis{Float32} = vbasis(vnorm(v3f(-2, 5, 20)),
                                   v3f(-1, 0, 0))
for name::Symbol in fieldnames(VBasis{Float32})
    from::v3f = getfield(BASIS_FROM, name)
    expected_to::v3f = getfield(BASIS_TO, name)
    @bp_test_no_allocations(
        begin
            rot = m3_look_at(BASIS_FROM, BASIS_TO)
            actual_to = m_apply_vector(rot, from)
            (vdist(expected_to, actual_to) < 0.0001)
        end,
        true,
        "m3_look_at(", BASIS_FROM, ", ", BASIS_TO, ") == ",
          m3_look_at(BASIS_FROM, BASIS_TO),
        "\nFrom: ", from,
        "\nExpected to: ", expected_to,
        "\nActual to: ", m_apply_vector(m3_look_at(BASIS_FROM, BASIS_TO), from),
    )
end