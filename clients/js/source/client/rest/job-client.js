import { post } from './rest-client'

export const saveJobsRoute = "/api/v1/job/"
export const deleteJobsRoute = "/api/v1/job/delete"

export const saveJobsRest = (clientId, job = {}) =>
    post(saveJobsRoute, {clientId, job});

export const deleteJobsRest = (clientId, job) =>
    post(deleteJobsRoute, {clientId, job});
