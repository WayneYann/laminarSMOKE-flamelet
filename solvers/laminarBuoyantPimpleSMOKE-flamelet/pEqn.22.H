
rho = Rho(p,psi);

volScalarField rAU(1.0/UEqn.A());
surfaceScalarField rhorAUf("Dp", fvc::interpolate(rho*rAU));
volVectorField HbyA("HbyA", U);
HbyA = rAU*UEqn.H();
phi.boundaryField() =
    fvc::interpolate(rho.boundaryField()*U.boundaryField())
  & mesh.Sf().boundaryField();

surfaceScalarField phig(-rhorAUf*ghf*fvc::snGrad(rho)*mesh.magSf());

surfaceScalarField phiHbyA
(
    "phiHbyA",
    fvc::interpolate(rho)
   *(
        (fvc::interpolate(HbyA) & mesh.Sf())
      + fvc::ddtPhiCorr(rAU, rho, U, phi)
    )
  + phig
);

fvOptions.relativeFlux(phiHbyA);

while (pimple.correctNonOrthogonal())
{
    fvScalarMatrix p_rghEqn
    (
        fvc::ddt(psi, rho)*gh
      + fvc::div(phiHbyA)
      + fvm::ddt(psi, p_rgh)
      - fvm::laplacian(rhorAUf, p_rgh)
     ==
        parcels.Srho()
      + surfaceFilm.Srho()
      + fvOptions(psi, p_rgh, rho.name())
    );

    fvOptions.constrain(p_rghEqn);

    p_rghEqn.solve(mesh.solver(p_rgh.select(pimple.finalInnerIter())));

    if (pimple.finalNonOrthogonalIter())
    {
        phi = phiHbyA + p_rghEqn.flux();
        U = HbyA + rAU*fvc::reconstruct((p_rghEqn.flux() + phig)/rhorAUf);
        U.correctBoundaryConditions();
        fvOptions.correct(U);
    }
}

p = p_rgh + rho*gh;

#include "rhoEqn.H"
{
    dimensionedScalar totalMass = fvc::domainIntegrate(rho);

    scalar sumLocalContErr =
        (fvc::domainIntegrate(mag(rho - Rho(p,psi)))/totalMass).value();

    scalar globalContErr =
        (fvc::domainIntegrate(rho - Rho(p,psi))/totalMass).value();

    cumulativeContErr += globalContErr;

    Info << "time step continuity errors : sum local = " << sumLocalContErr
         << ", global = " << globalContErr
         << ", cumulative = " << cumulativeContErr
         << endl;
}

K = 0.5*magSqr(U);

// Be careful
// In case of energy balance equation here you can calculate the DpDt contribution
// The present solver does not need it
