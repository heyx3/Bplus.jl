println("#TODO: Test the built-in buttons")

@bp_button Test{I:Integer, T} begin
    i::I = one(i)*2
    t::Optional{T}
    state::I = zero(I)

    Test{I}(t::T, i::I = one(i)*4) where {I<:Integer, T} = Test{I, T}(i=i, t=t)

    function RAW(b)
        b.state += 1
        return (b.state % b.i) == 0
    end
end

println("#TODO: Test Button_Test")


println("#TODO: Test the button serialization")