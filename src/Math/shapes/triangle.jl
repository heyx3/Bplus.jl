struct Triangle{N, F} <: AbstractShape{N, F}
    a::Vec{N, F}
    b::Vec{N, F}
    c::Vec{N, F}
    # property 'points::NTuple{3, Vec{N, F}}'
end
@inline Triangle(a::Vec{N, T1}, b::Vec{N, T2}, c::Vec{N, T3}) where {N, T1, T2, T3} =
    Triangle{N, promote_type(T1, promote_type(T2, T3))}(a, b, c)

@inline Base.getproperty(t::Triangle, n::Symbol) = getproperty(t, Val(n))
@inline Base.getproperty(t::Triangle, ::Val{F}) where {F} = getfield(t, F)
@inline Base.getproperty(t::Triangle, ::Val{:points}) = (t.a, t.b, t.c)
#TODO: Make this getproperty design pattern into a macro

#TODO: Query properties of a triangle