package common

import (
	"errors"
	"math"
)

func Cosine(a []float32, b []float32) (cosine float64, err error) {
	count := 0
	length_a := len(a)
	length_b := len(b)
	if length_a > length_b {
		count = length_a
	} else {
		count = length_b
	}
	sumA := 0.0
	s1 := 0.0
	s2 := 0.0
	for k := 0; k < count; k++ {
		if k >= length_a {
			s2 += math.Pow(float64(b[k]), 2)
			continue
		}
		if k >= length_b {
			s1 += math.Pow(float64(a[k]), 2)
			continue
		}
		sumA += float64(a[k]) * float64(b[k])
		s1 += math.Pow(float64(a[k]), 2)
		s2 += math.Pow(float64(b[k]), 2)
	}
	if s1 == 0 || s2 == 0 {
		return 0.0, errors.New("vectors should not be null (all zeros)")
	}
	val := sumA / (math.Sqrt(s1) * math.Sqrt(s2))
	return (1 + val) / 2, nil
}
