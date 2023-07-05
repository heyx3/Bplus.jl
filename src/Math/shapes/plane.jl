struct Plane{N, F} <: AbstractShape{N, F}
    origin::Vec{N, F}
    normal::Vec{N, F}
end

@inline Plane(origin::Vec{N, T1}, normal::Vec{N, T2}) where {N, T1, T2} =
    Plane{N, promote_type(T1, T2)}(origin, normal)