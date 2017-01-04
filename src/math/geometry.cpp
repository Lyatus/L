#include "geometry.h"

#include "math.h"

using namespace L;

bool L::lineLineIntersect(const Vector3f& p1, const Vector3f& p2,
                          const Vector3f& p3, const Vector3f& p4,
                          Vector3f* a, Vector3f* b) {
  Vector3f p43(p4-p3);
  float ls43(p43.lengthSquared());
  if(ls43<.0001f) return false;
  Vector3f p21(p2-p1);
  float ls21(p21.lengthSquared());
  if(ls21<.0001f) return false;
  Vector3f p13(p1-p3);
  float d1343(p13.dot(p43)),
    d4321(p43.dot(p21)),
    d1321(p13.dot(p21));
  float denom(ls21*ls43-d4321*d4321);
  if(abs(denom)<.0001f) return false;
  float numer(d1343*d4321-d1321*ls43);
  float mua(numer/denom);
  float mub((d1343+d4321*mua)/ls43);

  *a = p1 + p21*mua;
  *b = p3 + p43*mub;
  return true;
}
Matrix44f L::SQTToMat(const Quatf& q, const Vector3f& t, float s) {
  Matrix44f wtr;
  const float& x(q.x());
  const float& y(q.y());
  const float& z(q.z());
  const float& w(q.w());
  const float x2(x*x);
  const float y2(y*y);
  const float z2(z*z);
  wtr(0, 0) = (1.f - 2.f*y2 - 2.f*z2) * s;
  wtr(0, 1) = (2.f*x*y - 2.f*z*w) * s;
  wtr(0, 2) = (2.f*x*z + 2.f*y*w) * s;
  wtr(1, 0) = (2.f*x*y + 2.f*z*w) * s;
  wtr(1, 1) = (1.f - 2.f*x2 - 2.f*z2) * s;
  wtr(1, 2) = (2.f*y*z - 2.f*x*w) * s;
  wtr(2, 0) = (2.f*x*z - 2.f*y*w) * s;
  wtr(2, 1) = (2.f*y*z + 2.f*x*w) * s;
  wtr(2, 2) = (1.f - 2.f*x2 - 2.f*y2) *s;
  wtr(3, 0) = 0.f;
  wtr(3, 1) = 0.f;
  wtr(3, 2) = 0.f;
  wtr(0, 3) = t.x();
  wtr(1, 3) = t.y();
  wtr(2, 3) = t.z();
  wtr(3, 3) = 1.f;
  return wtr;
}
Matrix33f L::quatToMat(const Quatf& q) {
  Matrix33f wtr;
  const float& x(q.x());
  const float& y(q.y());
  const float& z(q.z());
  const float& w(q.w());
  const float x2(x*x);
  const float y2(y*y);
  const float z2(z*z);
  wtr(0, 0) = 1.f - 2.f*y2 - 2.f*z2;
  wtr(0, 1) = 2.f*x*y - 2.f*z*w;
  wtr(0, 2) = 2.f*x*z + 2.f*y*w;
  wtr(1, 0) = 2.f*x*y + 2.f*z*w;
  wtr(1, 1) = 1.f - 2.f*x2 - 2.f*z2;
  wtr(1, 2) = 2.f*y*z - 2.f*x*w;
  wtr(2, 0) = 2.f*x*z - 2.f*y*w;
  wtr(2, 1) = 2.f*y*z + 2.f*x*w;
  wtr(2, 2) = 1.f - 2.f*x2 - 2.f*y2;
  return wtr;
}
bool L::raySphereIntersect(const Vector3f& c, float r, const Vector3f& o, const Vector3f& d, float& t) {
  const float radiusSqr(sqr(r));
  const Vector3f oc(o - c);
  const float ddotoc(d.dot(oc));
  const float delta(ddotoc*ddotoc-oc.lengthSquared()+radiusSqr);
  t = -ddotoc-sqrt(delta);
  return t>=0;
}
bool L::rayBoxIntersect(const Interval3f& b, const Vector3f& o, const Vector3f& d, float& t, const Vector3f& id) {
  const float xmin = (b.min().x() - o.x())*id.x();
  const float xmax = (b.max().x() - o.x())*id.x();
  const float ymin = (b.min().y() - o.y())*id.y();
  const float ymax = (b.max().y() - o.y())*id.y();
  const float zmin = (b.min().z() - o.z())*id.z();
  const float zmax = (b.max().z() - o.z())*id.z();
  const float tmin = max(max(min(xmin, xmax), min(ymin, ymax)), min(zmin, zmax));
  const float tmax = min(min(max(xmin, xmax), max(ymin, ymax)), max(zmin, zmax));
  t = max(0.f, tmin);
  return (tmin<tmax && tmax>0.f);
}
